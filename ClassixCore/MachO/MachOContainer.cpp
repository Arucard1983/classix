//
// MachOContainer.cpp
// Classix
//

#include "MachOContainer.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <cstring>

namespace MachO
{
    MachOContainer::MachOContainer(Common::Allocator& allocator, const uint8_t* begin, const uint8_t* end)
        : allocator(allocator), dataBegin(begin), dataEnd(end), swapBytes(false), entryPoint(0)
    {
        if (!ValidateHeader()) {
            throw std::runtime_error("Ficheiro inválido ou não é um binário PowerPC Mach-O.");
        }
        ParseLoadCommands();
    }

    bool MachOContainer::ValidateHeader()
    {
        if (size() < sizeof(mach_header)) return false;

        const mach_header* header = reinterpret_cast<const mach_header*>(dataBegin);
        
        if (header->magic == 0xFEEDFACE) {
            swapBytes = false;
        } else if (header->magic == 0xCEFAEDFE) { 
            swapBytes = true;
        } else {
            return false;
        }

        int32_t cpuType = Read32(header->cputype);
        if (cpuType != 18) { // 18 = CPU_TYPE_POWERPC (32-bit)
            std::cerr << "[MachO Parser] Erro: Arquitetura não suportada (Esperava PowerPC 32-bit)" << std::endl;
            return false;
        }

        return true;
    }

    void MachOContainer::ParseLoadCommands()
    {
        const mach_header* header = reinterpret_cast<const mach_header*>(dataBegin);
        uint32_t numCommands = Read32(header->ncmds);
        
        const uint8_t* currentCmdPtr = dataBegin + sizeof(mach_header);
        
        for (uint32_t i = 0; i < numCommands; ++i) {
            // Proteção de limites contra cabeçalhos truncados
            if (currentCmdPtr + sizeof(load_command) > dataEnd) {
                std::cerr << "[MachO Parser] Erro crítico: Ficheiro terminou antes de ler todos os comandos." << std::endl;
                break;
            }

            const load_command* cmd = reinterpret_cast<const load_command*>(currentCmdPtr);
            uint32_t cmdType = Read32(cmd->cmd);
            uint32_t cmdSize = Read32(cmd->cmdsize);
            
            // Evita overflows aritméticos e loops infinitos se cmdSize for inválido (ex: 0)
            if (cmdSize < sizeof(load_command) || (currentCmdPtr + cmdSize) > dataEnd) {
                std::cerr << "[MachO Parser] Comando corrompido ou fora dos limites detetado. A abortar parsing." << std::endl;
                break;
            }

            if (cmdType == LC_LOAD_DYLIB) {
                if (cmdSize >= sizeof(dylib_command)) {
                    const dylib_command* dylibCmd = reinterpret_cast<const dylib_command*>(currentCmdPtr);
                    uint32_t offset = Read32(dylibCmd->name_offset);
                    
                    const uint8_t* stringPtr = currentCmdPtr + offset;
                    if (stringPtr >= dataBegin && stringPtr < dataEnd) {
                        // Calcula o tamanho máximo seguro para a string dentro deste comando
                        size_t maxLen = (currentCmdPtr + cmdSize) - stringPtr;
                        
                        // Determina o comprimento real parando no caractere nulo (\0)
                        size_t realLen = 0;
                        while (realLen < maxLen && stringPtr[realLen] != '\0') {
                            realLen++;
                        }

                        std::string libraryPath(reinterpret_cast<const char*>(stringPtr), realLen);
                        if (!libraryPath.empty()) {
                            importedLibraries.push_back(libraryPath);
#ifdef DEBUG_DISASSEMBLE
                            std::cout << "[MachO Parser] Dependência encontrada: " << libraryPath << std::endl;
#endif
                        }
                    }
                }
            }
            else if (cmdType == LC_SEGMENT) {
                if (cmdSize >= sizeof(segment_command)) {
                    const segment_command* segCmd = reinterpret_cast<const segment_command*>(currentCmdPtr);
                    
                    uint32_t vmaddr   = Read32(segCmd->vmaddr);
                    uint32_t vmsize   = Read32(segCmd->vmsize);
                    uint32_t fileoff  = Read32(segCmd->fileoff);
                    uint32_t filesize = Read32(segCmd->filesize);
                    int32_t  initprot = Read32(segCmd->initprot);

                    // Garante que o nome do segmento termina sempre com \0 (limite de 16 chars)
                    char segNameBuf[17] = {0};
                    std::memcpy(segNameBuf, segCmd->segname, 16);
                    std::string segmentName(segNameBuf);

                    // Validação cirúrgica de limites para mapeamento seguro de ficheiro
                    const uint8_t* segmentData = nullptr;
                    if (filesize > 0) {
                        if (dataBegin + fileoff + filesize <= dataEnd) {
                            segmentData = dataBegin + fileoff;
                        } else {
                            std::cerr << "[MachO Parser] Aviso: Segmento " << segmentName << " aponta para fora do ficheiro físico." << std::endl;
                        }
                    }

                    ParsedSegment segment = {
                        .name = segmentName,
                        .vmAddr = vmaddr,
                        .vmSize = vmsize,
                        .fileOffset = fileoff,
                        .fileSize = filesize,
                        .initProt = initprot,
                        .dataPtr = segmentData
                    };

                    segments.push_back(segment);

#ifdef DEBUG_DISASSEMBLE
                    std::cout << "[MachO Parser] Segmento Mapeado: " << segmentName 
                              << " -> VM: 0x" << std::hex << vmaddr 
                              << " (Tamanho VM: 0x" << vmsize << ")" << std::dec << std::endl;
#endif
                }
            }
            else if (cmdType == LC_UNIXTHREAD) {
                // Proteção para ler com segurança o flavor e o count do estado da thread
                if (cmdSize >= sizeof(load_command) + 8) {
                    const uint32_t* threadStatePtr = reinterpret_cast<const uint32_t*>(currentCmdPtr + sizeof(load_command));
                    
                    uint32_t flavor = Read32(threadStatePtr[0]); // 1 = PPC_THREAD_STATE
                    uint32_t count  = Read32(threadStatePtr[1]);
                    
                    // PPC_THREAD_STATE do OS X/Darwin possui tipicamente 40 dwords estruturados
                    if (flavor == 1 && count >= 40) {
                        // Verifica se o tamanho total do comando acomoda o array de registadores
                        if (cmdSize >= sizeof(load_command) + 8 + (count * sizeof(uint32_t))) {
                            // SRR0 (Instruction Pointer / Entry Point) é tradicionalmente o primeiro elemento
                            uint32_t srr0 = Read32(threadStatePtr[2]);
                            entryPoint = srr0;
#ifdef DEBUG_DISASSEMBLE
                            std::cout << "[MachO Parser] Entry Point detetado (SRR0): 0x" 
                                      << std::hex << entryPoint << std::dec << std::endl;
#endif
                        }
                    } else {
                        std::cerr << "[MachO Parser] Símbolo de execução 'thread flavor' (" << flavor << ") não suportado." << std::endl;
                    }
                }
            }
            
            // Avança para o próximo comando de carga usando o tamanho validado
            currentCmdPtr += cmdSize;
        }
    }
}

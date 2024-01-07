#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>


void verifyZero(uint32_t reg, uint32_t teste, uint64_t temp) {
  if (reg == 0) {
    if (teste == 1) {
      temp = temp & 0xFFFFFFFF;
    } else {
      temp = temp & 0xFFFFFFFF00000000;
    }
  }
}

int checkBit32(uint32_t reg, int BitPosition) {
  uint32_t mask = 1u << BitPosition;
  return (reg & mask) != 0;
}

int checkBit64(uint64_t reg, int BitPosition) {
  uint64_t mask = 1u << BitPosition;
  return (reg & mask) != 0;
}

uint32_t ExtensaoBit21To32(uint32_t hexa) {
  if (checkBit32(hexa, 20)) {
    return hexa | 0xFFF00000;
  } else {
    return hexa;
  } 
}

char *getRegisterSmaller(uint32_t reg) {
  char *result;
  if (reg == 28) {
    result = strdup("ir");
  } else if (reg == 29) {
    result = strdup("pc");
  } else if (reg == 30) {
    result = strdup("sp");
  } else if (reg == 31) {
    result = strdup("sr");
  } else {
    result = (char *)malloc(2 + snprintf(NULL, 0, "%u", reg));
    sprintf(result, "r%u", reg);
  }
  return result;  
}

char *getRegisterBigger(uint32_t reg) {
  char *result;
  if (reg == 28) {
    result = strdup("IR");
  } else if (reg == 29) {
    result = strdup("PC");
  } else if (reg == 30) {
    result = strdup("SP");
  } else if (reg == 31) {
    result = strdup("SR");
  } else {
    result = (char *)malloc(2 + snprintf(NULL, 0, "%u", reg));
    sprintf(result, "r%u", reg);
  }
  return result;  
}



int main(int argc, char *argv[])
{

    FILE *input = fopen(argv[1], "r");
    FILE *output = fopen(argv[2], "w");

    uint32_t R[32] = {0};

    uint8_t *MEM8 = (uint8_t *)(calloc(32, 1024));
    uint32_t *MEM32 = (uint32_t *)(calloc(32, 1024));

    printf("[START OF SIMULATION]\n");
    fprintf(output, "[START OF SIMULATION]\n");

    char row[200];
    uint32_t c = 0;
    while (fgets(row, sizeof(row), input) != NULL)
    {
        MEM32[c] = strtoul(row, NULL, 16);
        c++;
    }

    uint8_t executa = 1;
    while (executa)
    {
        char instrucao[30] = {0};

        uint8_t z = 0, x = 0, i = 0, y = 0, l = 0;
        uint32_t pc = 0, xyl = 0, ir = 0;
        uint64_t moment_1 = 0, moment_2 = 0, moment_3 = 0;

        R[28] = ((MEM8[R[29] + 0] << 24) | (MEM8[R[29] + 1] << 16) | (MEM8[R[29] + 2] << 8) | (MEM8[R[29] + 3] << 0)) | MEM32[R[29] >> 2];

        uint8_t opcode = (R[28] & (0b111111 << 26)) >> 26;

        switch (opcode)
        {
          // mov (tipe U)
          case 0b000000:
            pc = R[29];
            z = (R[28] & (0b11111 << 21)) >> 21;
            xyl = R[28] & 0x1FFFFF;
            R[z] = xyl;
            sprintf(instrucao, "mov r%u,%u", z, xyl);
            fprintf(output, "0x%08X:\t%-25s\tR%u=0x%08X\n", R[29], instrucao, z, xyl);
            printf("0x%08X:\t%-25s\tR%u=0x%08X\n", R[29], instrucao, z, xyl);
            break;

          //movs (tipe U)
          case 0b000001:
            pc = R[29];
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 21;
            y = (R[28] & (0b11111 << 11)) >> 11;
            xyl = R[28] & 0x1FFFFF;
            R[z] = ExtensaoBit21To32(xyl);

            sprintf(instrucao, "movs r%u, %i", z, R[z]);
            fprintf(output, "0x%08X:\t%-25s\tR%u=0x%08X\n", R[29], instrucao, z, R[z]);
            printf("0x%08X:\t%-25s\tR%u=0x%08X\n", R[29], instrucao, z, R[z]);
            break;

          //add (tipe U)
          case 0b000010:
            pc = R[29];
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >>16;
            y = (R[28] & (0b11111 << 11)) >> 11;
            R[z] = R[x] + R[y];

             //ZN(igual a zero) rz=0
            if (R[z] == 0) {
              R[31] = R[31] | 0b10000000;
            } else {
              R[31] = R[31] & ~0b10000000;
            }

            //SN(divisao por zero) rz31 =1
            if (R[z] < 0) {
              R[31] = R[31] | 0b0010000;
            } else {
              R[31] = R[31] & ~0b0010000;
            }

            //OV(overflow) rx31=ry31 and rz31!=rz31
            moment_1 = checkBit64(R[z], 31);
            moment_2 = checkBit64(R[y], 31);
            moment_3 = checkBit64(R[x], 31);

           if ((moment_1 == moment_2) && (moment_1 != moment_3)) {
             R[31] = R[31] | 0b0001000;
           } else {
             R[31] = R[31] & ~0b0001000;
            }

           //CY (carry) vai a um aritmetico
            if ((R[z] = R[z] & 0b1) != 0) {
              R[31] = R[31] | 0x100000000;
            } else {
              R[31] = R[31] & ~0x100000000;
            }

            //0x????????: add rz,rx,ir              Rz=Rx+IR=0x????????,SR=0x????????
            sprintf(instrucao, "add r%u, %s,%s", z, getRegisterSmaller(x), getRegisterSmaller(y));
            fprintf(output, "0x%08X:\t%-25s\tR%u=%s+%s=0x%08X,SR=0x%08X\n", pc, instrucao, z, getRegisterBigger(x), getRegisterBigger(y), R[z], R[31]);
            printf("0x%08X:\t%-25s\tR%u=%s+%s=0x%08X,SR=0x%08X\n", pc, instrucao, z, getRegisterBigger(x), getRegisterBigger(y), R[z], R[31]);
            break;

            uint8_t subcode = (R[28] & (0b111 << 8)) >> 8;

          //sla
          case 0b000100:

            switch (subcode) 
            {
              //mul (tipe U)
              case 0b000:

                pc = R[29];
                xyl = R[28] & 0b11111;
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >>16;
                y = (R[28] & (0b11111 << 11)) >> 11;

                // R[l] : R[z] = R[x] * R[y]
                moment_1 = ((uint64_t)(R[z]) << 32);
                moment_1 = moment_1 | ((uint64_t)(R[xyl]));
                moment_1 = ((uint64_t)(R[x]) * (uint64_t)(R[y]));

                verifyZero(x, 1, moment_1);
                verifyZero(y, 2, moment_1);

                // Extracting the 32 most significant bits
                R[xyl] = ((moment_1 >> 32) & 0xFFFFFFFF);

                // Extracting the least significant bits
                R[z] = ((moment_1) & 0xFFFFFFFF);

                //ZN rl rz = 0
                if (moment_1 == 0) {
                  R[31] = R[31] | 0b10000000;
                } else {
                  R[31] = R[31] & ~0b10000000;
                }
                
                // CY rl != 0 
                if (R[xyl] != 0) {
                  R[31] = R[31] | 0x100000000;
                } else {
                  R[31] = R[31] & ~0x100000000;
                }
                

                //	0x????????:	mul rl,rz,rx,ry          	Rl:Rz=Rx*Ry=0x????????????????,SR=0x????????
                sprintf(instrucao, "mul, r%u, r%u, r%u, r%u", xyl, z, x, y);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u*R%u=0x%08X,SR=0x%08X\n", pc, instrucao, xyl, z, x, y, R[z], R[31]);
                printf("0x%08X:\t%-25s\tR%u:R%u=R%u*R%u=0x%08X,SR=0x%08X\n", pc, instrucao, xyl, z, x, y, R[z], R[31]);

              //sll
              case 0b001:

                pc = R[29];
                xyl = R[28] & 0b11111;
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >> 16;
                y = (R[28] & (0b11111 << 11)) >> 11;

                // R[z] : R[x] = (R[z]:R[y]) * 2^{l+1}
                moment_1 = ((uint64_t)(R[z]) << 32);
                moment_1 = moment_1 | ((uint64_t)(R[y]));
                moment_2 = (uint64_t)(pow(2, l + 1));
                moment_1 = moment_1 * moment_2;
                verifyZero(z, 1, moment_1);
                verifyZero(y, 2, moment_1);

                // Extracting the 32 most significant bits
                R[z] = ((moment_1 >> 32) & 0xFFFFFFFF);

                // Extracting the least significant bits
                R[x] = ((moment_1) & 0xFFFFFFFF);

                //ZN(igual a zero) rz=0
                if (R[z] == 0) {
                  R[31] = R[31] | 0b1000000;
                } else {
                  R[31] = R[31] & ~0b10000000;
                }

                //CY rz != 0
                if (R[z] != 0) {
                  R[31] = R[31] | 0b1;
                } else {
                  R[31] = R[31] & ~0b1;
                }

                //	0x????????:	sll rz,rx,ry,u           	Rz:Rx=Rz:Ry<<u=0x????????????????,SR=0x????????
                sprintf(instrucao, "sll, r%u, r%u, r%u, %d", z, x, y, xyl);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u:R%u<<%d=0x%08X,SR=0x%08X\n", pc, instrucao, z, x, z, y, xyl, R[z], R[31]);
                printf("0x%08X:\t%-25s\tR%u:R%u=R%u:R%u<<%d=0x%08X,SR=0x%08X\n", pc, instrucao, z, x, z, y, xyl, R[z], R[31]);
                

              //muls 
              case 0b010:
                pc = R[29];
                xyl = R[28] & 0b11111;
                z = (R[28] & (0b11111 << 21)) >> 21;
                x = (R[28] & (0b11111 << 16)) >>16;
                y = (R[28] & (0b11111 << 11)) >> 11;

                // R[l] : R[z] = R[x] * R[y]
                moment_1 = ((uint64_t)(R[z]) << 32);
                moment_1 = moment_1 | ((uint64_t)(R[xyl]));
                moment_1 = ((uint64_t)(R[x]) * (uint64_t)(R[y]));

                verifyZero(x, 1, moment_1);
                verifyZero(y, 2, moment_1);

                // Extracting the 32 most significant bits
                R[xyl] = ((moment_1 >> 32) & 0xFFFFFFFF);

                // Extracting the least significant bits
                R[z] = ((moment_1) & 0xFFFFFFFF);

                 //ZN(igual a zero) rz=0
                if (moment_1 == 0) {
                    R[31] = R[31] | 0b1000000;
                  } else {
                    R[31] = R[31] & ~0b10000000;
                  }

                  //OV rl != 0
                if (R[xyl] != 0) {
                  R[31] = R[31] | 0b0001000;
                } else {
                  R[31] = R[31] & ~0b0001000;
                }


                //	0x????????:	muls rl,rz,rx,ry         	Rl:Rz=Rx*Ry=0x????????????????,SR=0x????????
                sprintf(instrucao, "muls, r%u, r%u, r%u, r%u", xyl, z, x, y);
                fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u*R%u=0x%08X,SR=0x%08X\n", pc, instrucao, xyl, z, x, y, R[z], R[31]);
                printf("0x%08X:\t%-25s\tR%u:R%u=R%u*R%u=0x%08X,SR=0x%08X\n", pc, instrucao, xyl, z, x, y, R[z], R[31]);
                break;
               
            

              }


            pc = R[29];
            xyl = R[28] & 0b11111;
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >> 16;
            y = (R[28] & (0b11111 << 11)) >> 11;

            // R[z] : R[x] = (R[z]:R[y]) * 2^{l+1}
            moment_1 = ((uint64_t)(R[z]) << 32);
            moment_1 = moment_1 | ((uint64_t)(R[y]));
            moment_2 = (uint64_t)(pow(2, l + 1));
            moment_1 = moment_1 * moment_2;

            verifyZero(z, 1, moment_1);
            verifyZero(y, 2, moment_1);

            // Extracting the 32 most significant bits
            R[z] = ((moment_1 >> 32) & 0xFFFFFFFF);

            // Extracting the least significant bits
            R[x] = ((moment_1) & 0xFFFFFFFF);

            //ZN(igual a zero) rz=0
            if (R[z] == 0) {
              R[31] = R[31] | 0b10000000;
            } else {
              R[31] = R[31] & ~0b10000000;
            }

            //OV rz != 0
            if (R[z] != 0) {
              R[31] = R[31] | 0b0001000;
            } else {
              R[31] = R[31] & ~0b0001000;
            }

            //	0x????????:	sla rz,rx,ry,u    	Rz:Rx=Rz:Ry<<u=0x????????????????,SR=0x????????
            sprintf(instrucao, "sla, r%u, r%u, r%u, %d", z, x, y, xyl);
            fprintf(output, "0x%08X:\t%-25s\tR%u:R%u=R%u:R%u<<%d=0x%08X,SR=0x%08X\n", pc, instrucao, z, x, z, y, xyl, R[z], R[31]);
            printf("0x%08X:\t%-25s\tR%u:R%u=R%u:R%u<<%d=0x%08X,SR=0x%08X\n", pc, instrucao, z, x, z, y, xyl, R[z], R[31]);
            break;


          //sub (tipe U)
          case 0b000011:
            pc = R[29];
            z = (R[28] & (0b11111 << 21)) >> 21;
            x = (R[28] & (0b11111 << 16)) >>16;
            y = (R[28] & (0b11111 << 11)) >> 11;
            R[z] = R[x] - R[y];

             //ZN(igual a zero) rz=0
            if (R[z] == 0) {
              R[31] = R[31] | 0b10000000;
            } else {
              R[31] = R[31] & ~0b10000000;
            }

            //SN(divisao por zero) rz31 =1
            if (R[z] < 0) {
              R[31] = R[31] | 0b0010000;
            } else {
              R[31] = R[31] & ~0b0010000;
            }

            //OV(overflow) rx31!=ry31 and rz31!=rz31
            moment_1 = checkBit64(R[z], 31);
            moment_2 = checkBit64(R[y], 31);
            moment_3 = checkBit64(R[x], 31);

            if ((moment_1 != moment_2) && (moment_1 != moment_3)) {
              R[31] = R[31] | 0b0001000;
            } else {
              R[31] = R[31] & ~0b0001000;
            }

            //CY (carry) vai a um aritmetico
            if ((R[z] = R[z] & 0b1) != 0) {
              R[31] = R[31] | 0x100000000;
            } else {
              R[31] = R[31] & ~0x100000000;
            }

            //0x????????: sub rz,pc,ry  Rz=PC-Ry=0x????????,SR=0x????????
            sprintf(instrucao, "sub r%u,%d,r%u", z, pc, y);
            fprintf(output, "0x%08X:\t%-25s\tR%u=%d-R%u=0x%08X,SR=0x%08X\n", pc, instrucao, z, pc, y, R[z], R[31]);
            printf("0x%08X:\t%-25s\tR%u=%d-R%u=0x%08X,SR=0x%08X\n", pc, instrucao, z, pc, y, R[z], R[31]);
            break;


          // l8
          case 0b011000:

             pc = R[29];
             z = (R[28] & (0b11111 << 21)) >> 21;
             x = (R[28] & (0b11111 << 16)) >> 16;
             i = R[28] & 0xFFFF;
             R[z] = MEM8[R[x] + i] | (((uint8_t *)(MEM32))[(R[x] + i) >> 2]);
             sprintf(instrucao, "l8 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
             fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%02X\n", R[29], instrucao, z, R[x] + i, R[z]);
             printf("0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%02X\n", R[29], instrucao, z, R[x] + i, R[z]);
             break;

          // l32
          case 0b011010:
             z = (R[28] & (0b11111 << 21)) >> 21;
             x = (R[28] & (0b11111 << 16)) >> 16;
             i = R[28] & 0xFFFF;
             R[z] = ((MEM8[((R[x] + i) << 2) + 0] << 24) | (MEM8[((R[x] + i) << 2) + 1] << 16) | (MEM8[((R[x] + i) << 2) + 2] << 8) | (MEM8[((R[x] + i) << 2) + 3] << 0)) | MEM32[R[x] + i];
             sprintf(instrucao, "l32 r%u,[r%u%s%i]", z, x, (i >= 0) ? ("+") : (""), i);
             fprintf(output, "0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%08X\n", R[29], instrucao, z, (R[x] + i) << 2, R[z]);
             printf("0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%08X\n", R[29], instrucao, z, (R[x] + i) << 2, R[z]);
             break;

          // bun
          case 0b110111:
              pc = R[29];
              R[29] = R[29] + ((R[28] & 0x3FFFFFF) << 2);
              sprintf(instrucao, "bun %i", R[28] & 0x3FFFFFF);
              fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);
              printf("0x%08X:\t%-25s\tPC=0x%08X\n", pc, instrucao, R[29] + 4);
              break;

          // int
          case 0b111111:
              executa = 0;
              sprintf(instrucao, "int 0");
              fprintf(output, "0x%08X:\t%-25s\tCR=0x00000000,PC=0x00000000\n", R[29], instrucao);
              printf("0x%08X:\t%-25s\tCR=0x00000000,PC=0x00000000\n", R[29], instrucao);
              break;


        default:
            printf("Instrucao desconhecida!\n");
            executa = 0;
        }

        R[29] = R[29] + 4;
    }

    printf("[END OF SIMULATION]\n");
    fprintf(output, "[END OF SIMULATION]\n");
    fclose(input);
    fclose(output);
    return 0;
}

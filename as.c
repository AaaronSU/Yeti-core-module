#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define MAGIC_NUMBER htobe64(0x4152434859302E30) // Check that // Checked
#define HEADER_SIZE 64

FILE *file;
size_t const MAX_SIZE = 1024 * 1024;
char string[255];
char *src,
    *src_dump;

long int *symbol_table,
    *symbol_ptr;

long long int token;
long long int line;

unsigned long long int current_address;
unsigned long long int address_data;
unsigned long long int size_data;
unsigned long long int address_code;
unsigned long long int size_code;
unsigned long long int address_parallel_on;
unsigned long long int core_number;
unsigned long long int size_total;

typedef enum
{
    LOADU = 255,
    LOADS = 1,
    LOADF = 2,
    LOADV = 3,
    LOADT = 4,
    LOADG = 5,
    STOREU = 6,
    STORES = 7,
    STOREF = 8,
    STOREV = 9,
    STORET = 10,
    STOREG = 11,
    MOV = 12,
    MOVU = 13,
    MOVS = 14,
    MOVF = 15,
    MOVV = 16,
    MOVT = 17,
    MOVG = 18,
    MOVUI = 19,
    MOVSI = 20,
    MOVFI = 21,
    MOVVI = 22,
    MOVTI = 23,
    MOVGI = 24,
    CVTUS = 25,
    CVTSU = 26,
    CVTUF = 27,
    CVTFU = 28,
    CVTSF = 29,
    CVTFS = 30,
    ADDU = 31,
    SUBU = 32,
    MULU = 33,
    DIVU = 34,
    MODU = 35,
    FMAU = 36,
    SQRTU = 37,
    LOGU = 38,
    INCU = 39,
    DECU = 40,
    ANDU = 41,
    ORU = 42,
    XORU = 43,
    SHLU = 44,
    SHRU = 45,
    ROLU = 46,
    RORU = 47,
    POPCNTU = 48,
    LMBU = 49,
    ADDS = 50,
    SUBS = 51,
    MULS = 52,
    DIVS = 53,
    MODS = 54,
    FMAS = 55,
    SQRTS = 56,
    LOGS = 57,
    ANDS = 58,
    ORS = 59,
    XORS = 60,
    SHLS = 61,
    SHRS = 62,
    ROLS = 63,
    RORS = 64,
    POPCNTS = 65,
    LMBS = 66,
    ADDF = 67,
    SUBF = 68,
    MULF = 69,
    DIVF = 70,
    FMAF = 71,
    SQRTF = 72,
    LOGF = 73,
    CMPU = 74,
    CMPS = 75,
    CMPF = 76,
    JE = 77,
    JNE = 78,
    JGE = 79,
    JL = 80,
    JLE = 81,
    JZ = 82,
    JNZ = 83,
    OUTU = 84,
    OUTS = 85,
    OUTF = 86,
    OUTA = 87,
    OUTB = 88,
    OUTX = 89,
    HLT = 90
} OPCODE;

enum
{
    IDENTIFIER = 256,
    ASCII,
    U64,
    I64,
    F64,
    CHAR,
    IMMEDIATE,
    REGISTER,
    COLON,
    DEFINE_OR_GET,
    SCALAR_UNSIGNED_INT_REGISTER,
    SCALAR_SIGNED_INT_REGISTER,
    SCALAR_FLOAT_REGISTER,
    VECTOR_UNSIGNED_INT_REGISTER,
    VECTOR_SIGNED_INT_REGISTER,
    VECTOR_FLOAT_REGISTER,
    DATA_ADDRESS,
    CODE_ADDRESS,
};

enum
{
    Token,
    Hash,
    Name,
    Value,
    SymbolSize
};

void tokenize()
{
    char *ch_ptr;
    while ((token = *src++))
    {
        if (token == '\n')
        {
            line++;
        }
        else if (token == '#')
            while (*src != 0 && *src != '\n')
                src++;
        else if (token == '\t')
        {
            continue;
        }
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_'))
        {
            ch_ptr = src - 1;
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_'))
                // use token store hash value
                token = token * 147 + *src++;

            // keep hash
            token = (token << 6) + (src - ch_ptr);
            symbol_ptr = symbol_table;
            // search same symbol in table
            while (symbol_ptr[Token])
            {
                if (token == symbol_ptr[Hash] && !memcmp((char *)symbol_ptr[Name], ch_ptr, (size_t)(src - ch_ptr)))
                {
                    token = symbol_ptr[Token];
                    return;
                }
                symbol_ptr = symbol_ptr + SymbolSize;
            }
            // add new symbol
            symbol_ptr[Name] = (long int)ch_ptr;
            symbol_ptr[Hash] = token;
            token = symbol_ptr[Token] = IDENTIFIER;

            // parti pour visualiser
            // int i = -1;
            // while (++i < src - ch_ptr)
            // {
            //     printf("%c", ch_ptr[i]);
            // }
            // printf("\n");
            // continue;
            return;
        }
        else if (token == ' ' || token == ',')
        {
            continue;
        }
        else if (token == '"')
        {
            ch_ptr = src;
            while ((*src != '"'))
            {
                string[src - ch_ptr] = *src;
                src++;
            }
            // printf("%s\n", string);
            src++;
            token = CHAR;
            return;
        }
        else if (token == ':')
        {
            token = COLON;
            return;
        }
        else if (token == '@')
        {
            token = DEFINE_OR_GET;
            return;
        }
        else
        {
            // printf("%c\n", (char)token);
            printf("not recognize %c\n", (char)token);
            exit(-1);
        }
    }
}

int init_vm()
{
    if (!(symbol_table = malloc(MAX_SIZE / 16)))
    {
        printf("could not malloc(%ld) for symbol_table\n", MAX_SIZE / 16);
        return -1;
    }
    memset(symbol_table, 0, MAX_SIZE / 16);
    return 0;
}

int load_src(char *file)
{
    int fd;
    long int cnt;
    // use open/read/close for bootstrap.
    if ((fd = open(file, 0)) < 0)
    {
        printf("could not open source code(%s)\n", file);
        return -1;
    }
    if (!(src = src_dump = malloc(MAX_SIZE)))
    {
        printf("could not malloc(%ld) for source code\n", MAX_SIZE);
        return -1;
    }
    if ((cnt = read(fd, src, MAX_SIZE - 1)) <= 0)
    {
        printf("could not read source code(%ld)\n", cnt);
        return -1;
    }
    src[cnt] = 0; // EOF
    close(fd);
    return 0;
}

void keyword()
{
    int i;
    src = "loadu loads loadf loadv loadt loadg "
          "storeu stores storef storev storet storeg "
          "mov movu movs movf movv movt movg "
          "movui movsi movfi movvi movti movgi "
          "cvtus cvtsu cvtuf cvtfu cvtsf cvtfs "
          "addu subu mulu divu modu fmau sqrtu logu incu decu "
          "andu oru xoru shlu shru rolu roru popcntu lmbu "
          "adds subs muls divs mods fmas sqrts logs "
          "ands ors xors shls shrs rols rors popcnts lmbs "
          "addf subf mulf divf fmaf sqrtf logf "
          "cmpu cmps cmpf je jne jge jl jle jz jnz "
          "outu outs outf outa outb outx hlt "
          "u0 u1 u2 u3 u4 u5 u6 u7 u8 u9 u10 u11 u12 u13 u14 u15 u16 "
          "u17 u18 u19 u20 u21 u22 u23 u24 u25 u26 u27 u28 u29 u30 u31 "
          "s0 s1 s2 s3 s4 s5 s6 s7 s8 s9 s10 s11 s12 s13 s14 s15 s16 "
          "s17 s18 s19 s20 s21 s22 s23 s24 s25 s26 s27 s28 s29 s30 s31 "
          "f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16 "
          "f17 f18 f19 f20 f21 f22 f23 f24 f25 f26 f27 f28 f29 f30 f31 "
          "v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 "
          "v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 "
          "t0 t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 t11 t12 t13 t14 t15 t16 "
          "t17 t18 t19 t20 t21 t22 t23 t24 t25 t26 t27 t28 t29 t30 t31 "
          "g0 g1 g2 g3 g4 g5 g6 g7 g8 g9 g10 g11 g12 g13 g14 g15 g16 "
          "g17 g18 g19 g20 g21 g22 g23 g24 g25 g26 g27 g28 g29 g30 g31 "
          "ascii u64 i64 f64 "
          "data code";
    // add keywords to symbol table
    tokenize();
    symbol_ptr[Token] = LOADU;
    symbol_ptr[Value] = 0;

    i = LOADS;
    while (i <= HLT)
    {
        tokenize();
        symbol_ptr[Token] = i;
        symbol_ptr[Value] = i++;
    }
    i = 0;
    int opcode = SCALAR_UNSIGNED_INT_REGISTER;
    while (opcode <= VECTOR_FLOAT_REGISTER)
    {
        while (i < 32)
        {
            tokenize();
            symbol_ptr[Token] = opcode;
            symbol_ptr[Value] = i++;
        }
        i = 0;
        opcode++;
    }
    i = ASCII;
    while (i <= F64)
    {
        tokenize();
        symbol_ptr[Token] = i;
        symbol_ptr[Value] = i++;
    }
    tokenize();
    symbol_ptr[Token] = DATA_ADDRESS;
    tokenize();
    symbol_ptr[Token] = CODE_ADDRESS;

    src = src_dump;
}

void assert_token(long long int tk)
{
    if (token != tk)
    {
        printf("line %lld: expect token: %lld(%c), get: %lld(%c)\n", line, tk, (char)tk, token, (char)token);
        exit(-1);
    }
    printf("token: %lld\n", token);
    tokenize();
}

void write_immediate(unsigned long long int immediate)
{
    unsigned long long int to_write[1] = {htobe64(immediate)};
    current_address += 8;
    size_code += 8;
    fwrite(to_write, sizeof(unsigned long long int), 1, file);
}

void write_instruction(long long int opcode, long int reg1, long int reg2, long int reg3, long int offset)
{
    if (opcode < 0 || opcode >= 1 << 8)
    {
        printf("opcode not available: %lld", opcode); // warning mieux
    }
    int to_write[1] = {(int)(htobe32((__uint32_t)opcode << 24) + (offset << 15) + (reg1 << 10) + (reg2 << 5) + reg3)};
    current_address += 4;
    size_code += 4;
    fwrite(to_write, sizeof(int), 1, file);
    printf("reg1: %ld, reg2: %ld, reg3: %ld, offset: %ld\n", reg1, reg2, reg3, offset);
}

void parse_data_section()
{
    if (token == ASCII)
    {
        tokenize();
        assert_token(DEFINE_OR_GET);
        assert_token(IDENTIFIER);
        assert_token(CHAR);
        symbol_ptr[Value + SymbolSize] = (long int)(address_data + size_data);
        // printf("address data: %lld, size data: %lld", address_data, size_data);
        int string_ptr = 0;
        unsigned long long int len = 0;
        char previous_char;
        char END_LINE = (char)'\n';
        char END_CHAR = (char)0;
        while (string[string_ptr] != 0)
        {
            if (string[string_ptr] == '\\')
            {
                previous_char = string[string_ptr];
                string_ptr++;
                continue;
            }
            else if (string[string_ptr] == 'n' && previous_char == '\\')
            {
                fwrite(&END_LINE, sizeof(char), 1, file);
                previous_char = 0;
                string_ptr++;
                len++;
                continue;
            }
            fwrite(&string[string_ptr], sizeof(char), 1, file);
            string_ptr++;
            len++;
        }
        // printf("%d\n", strlen(string));
        // fwrite(string, sizeof(char), strlen(string), file);
        // fprintf(file, "%s", string);
        fwrite(&END_CHAR, sizeof(char), 1, file);
        size_data += len + 1;
        current_address += len + 1;
    }
    else
    {
        printf("not implemented yet the following token: %lld\n", token);
        exit(-1);
    }
    return;
}

void parse_code_section()
{
    long int reg1, val, opcode = token;
    if (token == MOVUI)
    {
        tokenize();
        if (token == SCALAR_UNSIGNED_INT_REGISTER)
        {
            reg1 = symbol_ptr[Value];
        }
        assert_token(SCALAR_UNSIGNED_INT_REGISTER);
        if (token == DEFINE_OR_GET)
        {
            assert_token(DEFINE_OR_GET); // change to tokenize
            if (token == IDENTIFIER)
            {
                val = symbol_ptr[Value];
                printf("opcode: %lx\n", opcode);

                printf("identifiant adress: %lx\n", val);
                printf("reg1: %lx\n", reg1);
                assert_token(IDENTIFIER); // change to tokenize
                write_instruction(opcode, reg1, 0, 0, 0);
                write_immediate((long long unsigned int)val);
            }
        }
        else
        {
            printf("not implemented yet the following token: %lld\n", token);
            exit(-1);
        }
    }
    else if (token == OUTB)
    {
        tokenize();
        if (token == SCALAR_UNSIGNED_INT_REGISTER)
        {
            reg1 = symbol_ptr[Value];
            write_instruction(opcode, reg1, 0, 0, 0);
        }
        assert_token(SCALAR_UNSIGNED_INT_REGISTER); // change to tokenize
    }
    else if (token == HLT)
    {
        write_instruction(opcode, 0, 0, 0, 0);
        tokenize();
    }
    else
    {
        printf("not implemented yet the following token: %lld\n", token);
        exit(-1);
    }
    return;
}

void parse()
{
    // printf("%s\n", src);
    printf("token: %lld\n", token);
    if (token == DATA_ADDRESS || token == CODE_ADDRESS)
    {
        long int previous_token = token;
        tokenize();
        assert_token(COLON);
        if (previous_token == DATA_ADDRESS)
        {
            address_data = current_address;
        }
        else
        {
            address_code = current_address;
        }
        parse();
    }
    else if (token == IDENTIFIER)
    {
        tokenize();
        assert_token(COLON);
        parse();
    }
    else if (token == ASCII || token == F64 || token == I64 || token == U64)
    {
        parse_data_section();
        parse();
    }
    else if ((token >= LOADS && token <= HLT) || token == LOADU)
    {
        parse_code_section();
        parse();
    }
    else if (token == 0)
    {
        return;
    }
    else
    {
        printf("unrecogni token : %lld\n", token);
        return;
    }
}

void write_header()
{
    fseek(file, 0, SEEK_SET);
    // address_data = HEADER_SIZE;
    // address_code = address_data + size_data;
    // unsigned long long int thread_number = 0;

    // size_total = address_code + size_code;
    address_parallel_on = current_address;
    core_number = 1;
    size_total = current_address;
    printf("Adress Data: %llx\nSize Data: %llx\nAdress Code: %llx\nSize Code: %llx\nTotal size: %llx\n", address_data, size_data, address_code, size_code, size_total);

    unsigned long long int to_write[8] = {
        MAGIC_NUMBER,                 // ARCHY0.0
        htobe64(address_data),        // Data section address
        htobe64(size_data),           // Data section size
        htobe64(address_code),        // Code section address
        htobe64(size_code),           // Code section size
        htobe64(address_parallel_on), // Parallel on section address
        htobe64(core_number),
        htobe64(size_total) // Size total
    };
    fwrite(to_write, sizeof(char), HEADER_SIZE, file); // With 8 sizeof i64 (8 bytes)
    return;
}

int main(int argc, char **argv)
{
    file = fopen("try.archy", "w");
    if (argc != 2)
    {
        printf("NaNANAANA");
        exit(-1);
    }
    if (load_src(*(argv + 1)) != 0)
        return -1;
    // init memory & register
    if (init_vm() != 0)
        return -1;
    fseek(file, HEADER_SIZE, SEEK_SET);
    current_address = HEADER_SIZE;
    // prepare keywords for symbol table
    keyword();
    // while (*src != 0)
    // {
    //     // printf("%s\n", src);
    //     tokenize();
    //     printf("token: %lld\n", token);
    // }
    // src = src_dump;
    // printf("\n");

    tokenize();
    parse();

    write_header();

    // symbol_ptr = symbol_table;
    // while (symbol_ptr[Token])
    // {
    //     printf("%ld, %s, %ld\n\n", symbol_ptr[Token], (char *)symbol_ptr[Name], symbol_ptr[Value]);
    //     symbol_ptr = symbol_ptr + SymbolSize;
    // }
    // printf("%c, %c", 0x5c, 0x6e);
    free(src_dump);
    fclose(file);

    return 0;
}
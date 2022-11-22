#include <stdio.h>

#define REG_SIZE 32
// 컴퓨터 구조 3조
// 컴퓨터 전자 시스템 공학부 201904536 최동근
// 프랑스학과 2016022331 이재영
typedef union iType //명령어 타입
{
    unsigned int I;
    struct rtype
    {
        unsigned int fn : 6;
        unsigned int sh : 5;
        unsigned int rd : 5;
        unsigned int rt : 5;
        unsigned int rs : 5;
        unsigned int opc : 6;
    }RT;
    struct itype
    {
        unsigned int opr : 16;
        unsigned int rt : 5;
        unsigned int rs : 5;
        unsigned int opc : 6;
    }IT;
    struct jtype
    {
        unsigned int addr : 26;
        unsigned int opc : 6;
    }JT;
}IR;

//ALU
int AddSubtract(int x, int y, int c0)
{
    int ret;
    c0 %= 2;
    if (c0 < 0 || c0 > 1)
    {
        printf("error in add / subract operation\n");
        exit(1);
    }

    if (c0 == 0){ //add
        ret = x + y;
    }
    else{ //subtract
        ret = x - y;
    }

    return ret;
}

int LogicOperation(int x, int y, int c10) {
    if (c10 < 0 || c10 > 3) {
        printf("(error) Logic oper\n");
        exit(1);
    }

    if (c10 == 0) { //AND
    return x & y;
    }else if (c10 == 1){ //OR
        return x | y;
    }
    else if (c10 == 2){ //XOR
        return x ^ y;
    }
    else{ //NOR
        return ~(x | y);
    }
}

int CheckZero(int CZ)
{
    return CZ == 0;
}

int CheckSetLess(int x, int y)
{
    return x < y;
}

int ShiftOperation(int v, int y, int c10)
{
    int ret;

    if (c10 < 0 || c10 > 3)
    {
        printf("error in shift operation\n");
        exit(1);
    }

    if (c10 == 0){ //No shift
        ret = v;
    }
    else if (c10 == 1){ //Shift left logical
        ret = v << y;
    }
    else if (c10 == 2){ //Shift right logical
        ret = (unsigned int)v >> y;
    }
    else{  //Shift right arithmetic
        ret = v >> y;
    }

    return ret;
}



int ALU(int x, int y, int i, int *ChReg)
{
    int c32 = (i >> 2) & 3, c10 = i & 3, ret;

    if (c32 == 0){ //shift
        ret = ShiftOperation(x, y, c10);
    }
    else if (c32 == 1){ //set less
        ret = CheckSetLess(x, y);
    }
    else if (c32 == 2){ //add/subtract
        ret = AddSubtract(x, y, c10);
        *ChReg = CheckZero(ret);
    }
    else{ //logic
        ret = LogicOperation(x, y, c10);
    }

    return ret;
}
//메모리
unsigned char PN[0x100000];
unsigned char DM[0x100000];
unsigned char SM[0x100000];

unsigned int MEM(unsigned int a, int v, int nRW, int S)
{
    unsigned int cell, offset;
    unsigned char *MP;
    cell = a >> 20; offset = a & 0xFFFFF;
    if (cell == 0x004){ //program memory
        MP = PN;
    }
    else if (cell == 0x100){ //data memory
        MP = DM;
    }
    else if (cell == 0x7FF){ //stack
        MP = SM;
    }
    else
    {
        printf("No memory\n");
        return 1;
    }

    MP += offset; //읽거나 쓸 메모리 주소 설정
    if (S == 0) //byte
    {
        if (nRW == 0) //read
        {
            return (char)MP[0];
        }
        else if (nRW == 1) //write
        {
            MP[0] = v;

            return 0;
        }
    }
    else if (S == 1) //half word
    {
        if (offset % 2 != 0)
        {
            printf("no address available\n");
            return 1;
        }

        if (nRW == 0) //read
        {
            return (short)(MP[0] << 8) + MP[1]; //Big Endian 방식으로 읽기
        }
        else if (nRW == 1) //write
        {
            MP[0] = v >> 8; MP[1] = v; //Big Endian 방식으로 쓰기

            return 0;
        }
    }
    else if (S == 2) //word
    {
        if (offset % 4 != 0)
        {
            printf("no address available\n");
            return 1;
        }

        if (nRW == 0) //read
        {
            unsigned int value = (MP[0] << 24) + (MP[1] << 16) + (MP[2] << 8) + MP[3];
            return value;
        }
        else if (nRW == 1) //write
        {
            MP[0] = v >> 24;
            MP[1] = v >> 16;
            MP[2] = v >> 8;
            MP[3] = v;
            return 0;
        }
    }
    else //S가 유효하지 않은 값일 경우 오류
    {
        printf("Invalid S value\n");
        return 1;
    }
}



//레지스터
unsigned int R[REG_SIZE];
unsigned int PC = 0, H = 0, L = 0; //레지스터, 프로그램 카운터, HI/LO

unsigned int REG(unsigned int A, unsigned int V, int nRW){
    if (A > 31){
        printf("Unavailable Register Value\n");
        return 1;
    }

    if (nRW == 0){ //레지스터 읽기
        return R[A];
    }
    else if (nRW == 1){ //레지스터 쓰기
        R[A] = V;
    }
    else{
        printf("Unavailable value\n");
        return 1;
    }
    return 0;
}

void ShowRegister(void){ //레지스터 출력

    printf("[REGISTER]\n");
    for (int i = 0; i < REG_SIZE; i++)
        printf("R %d = %x\n", i, REG(i, 0, 0));
    printf("PC = %x\n", PC);
    printf("HI = %x	LO: %x\n", H, L);
}

void SetPC(unsigned int val){ //PC 설정
    PC = val;
}

//step
unsigned int GetOp(IR ir){
    return ir.RT.opc;
}
unsigned int GetRs(IR ir)
{
    return ir.RT.rs;
}
unsigned int GetRt(IR ir)
{
    return ir.RT.rt;
}
unsigned int GetRd(IR ir)
{
    return ir.RT.rd;
}
unsigned int GetSh(IR ir)
{
    return ir.RT.sh;
}
unsigned int GetFn(IR ir)
{
    return ir.RT.fn;
}
unsigned int GetOperand(IR ir)
{
    return ir.IT.opr;
}
unsigned int GetOffset(IR ir)
{
    return ir.JT.addr;
}

void step(void)
{
    if (PC == 0){
        SetPC(0x400000);
    }
    printf("PC : %x	", PC);
    IR Instruction;
    Instruction.I = MEM(PC, 0, 0, 2);

    unsigned int op = GetOp(Instruction),
    rs = GetRs(Instruction),
    rt = GetRt(Instruction),
    rd = GetRd(Instruction),
    sh = GetSh(Instruction),
    fn = GetFn(Instruction),
    operand = GetOperand(Instruction),
    offset = GetOffset(Instruction);

    int ChReg = 0;

    PC += 4;
    switch (op) //명령어 해석 및 실행
    {
        case 001:
            printf("||\t bltz   &%d, %d \t||", Instruction.IT.rs, Instruction.IT.opr * 4);
            if (ALU(REG(rs, 0, 0), 0, 0x4, 1) == 1){
                SetPC(PC + offset * 4 - 4);
            } break;
        case 002:
            printf("||\t j      0x%08x \t||", Instruction.JT.addr * 4);
            SetPC(offset * 4);
            break;
        case 003:
            printf("||\t jal    0x%08x \t||", Instruction.JT.addr * 4);
            REG(31, PC, 1); SetPC(offset * 4);
            break;
        case 004:
            printf("||\t beq    $%d, $%d, %d \t||", Instruction.IT.rs, Instruction.IT.rt, Instruction.IT.opr * 4);
            if (ALU(REG(rs, 0, 0), REG(rt, 0, 0), 0xE, &ChReg) == 0){
                SetPC(PC + operand * 4 - 4);
            } break;
        case 005:
            printf("||\t bne    $%d, $%d, %d \t||", Instruction.IT.rs, Instruction.IT.rt, Instruction.IT.opr * 4);
            if (ALU(REG(rs, 0, 0), REG(rt, 0, 0), 0xE, &ChReg) != 0){
                SetPC(PC + operand * 4 - 4);
            } break;

        case 010:
            printf("||\t addi   $%d, $%d, %d \t||", Instruction.IT.rt, Instruction.IT.rs, (short)Instruction.IT.opr);
            REG(rt, ALU(REG(rs, 0, 0), (short)operand, 0x8, &ChReg), 1);
            break;
        case 012:
            printf("||\t slti   $%d, $%d, %d \t||", Instruction.IT.rt, Instruction.IT.rs, (short)Instruction.IT.opr);
            REG(rt, ALU(REG(rs, 0, 0), (short)operand, 0x4, &ChReg), 1);
            break;
        case 014:
            printf("||\t andi   $%d, $%d, %d \t||", Instruction.IT.rt, Instruction.IT.rs, (short)Instruction.IT.opr);
            REG(rt, ALU(REG(rs, 0, 0), (short)operand, 0xC, &ChReg), 1);
            break;
        case 015:
            printf("||\t ori    $%d, $%d, %d \t||", Instruction.IT.rt, Instruction.IT.rs, (short)Instruction.IT.opr);
            REG(rt, ALU(REG(rs, 0, 0), (short)operand, 0xD, &ChReg), 1);
            break;
        case 016:
            printf("||\t xori   $%d, $%d, %d \t||", Instruction.IT.rt, Instruction.IT.rs, (short)Instruction.IT.opr);
            REG(rt, ALU(REG(rs, 0, 0), (short)operand, 0xE, &ChReg), 1);
            break;
        case 017:
            printf("||\t lui    $%d, %d \t||", Instruction.IT.rt, (short)Instruction.IT.opr);
            REG(rt, (short)operand << 16, 1);
            break;

        case 040:
            printf("||\t lb     $%d, %d($%d) \t||", Instruction.IT.rt, Instruction.IT.opr, Instruction.IT.rs);
            REG(rt, (int)MEM(REG(rs, 0, 0) + operand, 0, 0, 2), 1);
            break;
        case 043:
            printf("||\t lw     $%d, %d($%d) \t||", Instruction.IT.rt, Instruction.IT.opr, Instruction.IT.rs);
            REG(rt, MEM(REG(rs, 0, 0) + operand, 0, 0, 2), 1);
            break;
        case 044:
            printf("||\t lbu    $%d, %d($%d) \t||", Instruction.IT.rt, Instruction.IT.opr, Instruction.IT.rs);
            REG(rt, (unsigned int)MEM(REG(rs, 0, 0) + operand, 0, 0, 2), 1);
            break;

        case 050:
            printf("||\t sb     $%d, %d($%d) \t||", Instruction.IT.rt, Instruction.IT.opr, Instruction.IT.rs);
            MEM(REG(rs, 0, 0) + operand, (int)REG(rt, 0, 0), 1, 2);
            break;
        case 053:
            printf("||\t sw     $%d, %d($%d) \t||", Instruction.IT.rt, Instruction.IT.opr, Instruction.IT.rs);
            MEM(REG(rs, 0, 0) + operand, REG(rt, 0, 0), 1, 2);
            break;

        default: //opc에 해당하지 않는 경우
            switch (fn){

                case 000:
                    printf("||\t sll    $%d, $%d, %d \t||" , Instruction.RT.rd, Instruction.RT.rt, Instruction.RT.sh);
                    REG(rd, ALU(REG(rt, 0, 0), sh, 0x1, &ChReg), 1);
                    break;
                case 002:
                    printf("||\t srl    $%d, $%d, %d \t||" , Instruction.RT.rd, Instruction.RT.rt, Instruction.RT.sh);
                    REG(rd, ALU(REG(rt, 0, 0), sh, 0x2, &ChReg), 1);
                    break;
                case 003:
                    printf("||\t sra    $%d, $%d, %d \t||" , Instruction.RT.rd, Instruction.RT.rt, Instruction.RT.sh);
                    REG(rd, ALU(REG(rt, 0, 0), sh, 0x3, &ChReg), 1);
                    break;

                case 010:
                    printf("||\t jr     $%d \t||", Instruction.RT.rs);
                    SetPC(REG(31, 0, 0));
                    break;
                case 014:
                    printf("||\t syscall %d \t\t||", REG(2, 0, 0));
                    if (REG(2, 0, 0) == 10) {
                        SetPC(0);
                    }
                    break;

                case 020:
                    printf("||\t mfhi   &%d \t||", Instruction.RT.rs);
                    H = rs;
                    break;
                case 022:
                    printf("||\t mflo   &%d \t||", Instruction.RT.rs);
                    L = rs;
                    break;

                case 030:
                    printf("||\t mul    $%d, $%d, $%d \t||", Instruction.RT.rd, Instruction.RT.rs, Instruction.RT.rt);
                    REG(rd, rs*rt, 1);
                    break;

                case 040:
                    printf("||\t add    $%d, $%d, $%d \t||", Instruction.RT.rd, Instruction.RT.rs, Instruction.RT.rt);
                    REG(rd, ALU(REG(rs, 0, 0), REG(rt, 0, 0), 0x8, &ChReg), 1);
                    break;
                case 042:
                    printf("||\t sub    $%d, $%d, $%d \t||", Instruction.RT.rd, Instruction.RT.rs, Instruction.RT.rt);
                    REG(rd, ALU(REG(rs, 0, 0), REG(rt, 0, 0), 0x9, &ChReg), 1);
                    break;
                case 044:
                    printf("||\t and    $%d, $%d, $%d \t||", Instruction.RT.rd, Instruction.RT.rs, Instruction.RT.rt);
                    REG(rd, ALU(REG(rs, 0, 0), REG(rt, 0, 0), 0xC, &ChReg), 1);
                    break;
                case 045:
                    printf("||\t or     $%d, $%d, $%d \t||", Instruction.RT.rd, Instruction.RT.rs, Instruction.RT.rt);
                    REG(rd, ALU(REG(rs, 0, 0), REG(rt, 0, 0), 0xD, &ChReg), 1);
                    break;
                case 046:
                    printf("||\t xor    $%d, $%d, $%d \t||", Instruction.RT.rd, Instruction.RT.rs, Instruction.RT.rt);
                    REG(rd, ALU(REG(rs, 0, 0), REG(rt, 0, 0), 0xE, &ChReg), 1);
                    break;
                case 047:
                    printf("||\t nor    $%d, $%d, $%d \t||", Instruction.RT.rd, Instruction.RT.rs, Instruction.RT.rt);
                    REG(rd, ALU(REG(rs, 0, 0), REG(rt, 0, 0), 0xF, &ChReg), 1);
                    break;

                case 052:
                    printf("||\t slt    $%d, $%d, $%d \t||", Instruction.RT.rd, Instruction.RT.rs, Instruction.RT.rt);
                    REG(rd, ALU(REG(rs, 0, 0), REG(rt, 0, 0), 0x4, &ChReg), 1);
                    break;

                        default:
                            printf("||\t Undefined Instructionruction \t||\n");
                            break;
                    }
            }
            printf("\n");
}

//파일 로드
void load()
{
    FILE *Fp = NULL;
    unsigned char temp[1024];
    unsigned int index = 0;
    unsigned int Instruction_Num = 0, data_Num = 0;
    char str[20];


    scanf("%s", &str); //파일명 입력
    if (fopen_s(&Fp, str, "rb")) //파일 오픈 오류시
    {
        printf("File Error!\n"); //오류 메시지 출력
        return 1;
    }

    for (int i = 0; fread(&temp[i], sizeof(temp), 1, Fp); i++); //1바이트씩 불러와 배열에 저장
    for (; index < 4; index++){ //첫 4바이트는 명령어의 개수
        Instruction_Num = Instruction_Num << 8;
        Instruction_Num += temp[index];
    }
    for (; index < 8; index++){ //그 다음 4바이트는 데이터의 개수
        data_Num = data_Num << 8;
        data_Num += temp[index];
    }

    for (int i = 0; i < Instruction_Num; i++){ //명령어 프로그램메모리에 저장

        int Instruction = 0;

        for (int j = index; j < index + 4; j++){
            Instruction = Instruction << 8;
            Instruction += temp[j];
        }
        index += 4;
        MEM(0x400000 + i * 4, Instruction, 1, 2);
    }
    for (int i = 0; i < data_Num; i++){ //데이터 데이터메모리에 저장
        int data = 0;
        for (int j = index; j < index + 4; j++){
            data = data << 8;
            data += temp[j];
        }
        index += 4;
        MEM(0x10000000 + i * 4, data, 1, 2);
    }
    SetPC(0x400000); //PC 초기화
    REG(29, 0x80000000, 1); //스택 포인터 초기화
    fclose(Fp);
}



int main()
{
    unsigned int breakPoint = 0;

    while (1)
    {
        char command[3];
        unsigned int pc, start, end, value;
        int num;

        scanf("%s", &command);
        switch (command[0])
        {
            case 'l': //실행파일 로드
                load();
                break;

            case 'j': //PC 설정
                scanf("%x", &pc);
                SetPC(pc);
                break;

            case 'g': //명령어 끝까지 실행
                if (PC == 0 || PC == breakPoint)  //syscall 10을 수행하여 프로그램이 종료된 상태 또는 브레이크 포인트를 만난 상태에서 다시 실행
                    step();
                while (PC != 0 && PC != breakPoint) //프로그램 끝까지 돌거나 브레이크 포인트 만날 때까지 진행
                    step();
                break;

            case 's':
                if (command[1] == 'r') //레지스터 값 설정
                {
                    scanf("%d %x", &num, &value);
                    REG(num, value, 1);
                }
                else if (command[1] == 'm') //메모리 값 설정
                {
                    scanf("%x %x", &num, &value);
                    MEM(num, value, 1, 2);
                }
                else //스텝
                    step();
                break;

            case 'm': //메모리 범위 출력
                scanf("%x %x", &start, &end);
                for (unsigned int i = start; i <= end; i += 4)
                    printf("MEM[%x] = %08x\n", i, MEM(i, 0, 0, 2));
                break;

            case 'r': //레지스터 상태 출력
                ShowRegister();
                break;

            case 'b': //브레이크 포인트 설정
                scanf("%x", &pc);
                breakPoint = pc;
                break;

            case 'x': //정상 종료
                printf("Successfully exit.\n");
                return 0;
        }
    }
}

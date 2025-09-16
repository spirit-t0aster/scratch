#include <conio.h>
#include <stdio.h>

static int mem[4000];
static int stk[512];
static int sp;
static int* tib = mem;
static int* in = &mem[128];
static int* tibc = &mem[129];

static int here;
static int latest;

void dpush(int s) {
    if(sp == 512) { fprintf(stderr, "stack overflow"); exit(0); }
    stk[sp] = s;
    sp++;
}

int dpop() {
    if(sp == 0) { fprintf(stderr, "stack underflow"); exit(0); }
    sp--;
    return stk[sp];
}

void key() {
    int c = _getch();
    if(c == 27) { exit(0); }
    dpush(c);
}

void emit() {
    int c = dpop();
    _putch(c);
}

void accept() {
    (*in) = 0;
    (*tibc) = 0;
    while(1) {
        key();
        int ch = dpop();
        if(ch == '\r') {
            printf("\n");
            //printf("\n>in is %lld\n", in-tib);
            break;
        }
        if(*tibc >= 128) { continue; }
        tib[*tibc] = ch;
        (*tibc)++;
        _putch(ch);
    }
    printf("\naccept: tibc: %d\n", *tibc);
}

void end_of_name() {
    if(*in >= 128 || *in >= *tibc || tib[*in] == ' ') {
        dpush(-1);
        return;
    }
    dpush(0);
}

int bl() {
    if(tib[*in] <= ' ') {
        return -1;
    }
    return 0;
}

int inbounds() {
    if(*in < *tibc) return 1;
    return 0;
}

// "<spaces>name<space> -- c-addr u"
int parse_name() {
    while(bl() && inbounds()) { (*in)++; } // skip leading spaces
    dpush(*in);
    while(!bl() && inbounds()) { (*in)++; } // parse name
    int old_in = dpop();
    int cnt = *in - old_in;
    //if(cnt < 1) return 0;
#if 0
    printf("parse_name: c-addr %u cnt %d '", old_in, cnt);
    int i;
    for(i=0;i<cnt;i++) {
        printf("%c", tib[old_in+i]);
    }
    printf("'\n");
#endif
    dpush(old_in);
    dpush(cnt);
    return cnt;
}

void test_create(char* name, int hidden, int imm) {
    mem[here] = latest;
    latest = here;
    here++;
    mem[here++] = strlen(name) & 0x3fffffff | hidden << 30 | imm << 31;
    int i;
    for(i=0;i<strlen(name);i++) {
        mem[here++] = name[i];
    }
}

// ( c-addr u -- c-addr 0 | xt 1 | xt -1 )
void find_name() {
    int cnt = dpop();
    int in = dpop();
    printf("find_name: '");
    int i;
    for(i=0;i<cnt;i++) {
        printf("%c", tib[in+i]);
    }
    printf("'\n");
    int cur = latest;
    while(cur) {
        int next = mem[cur];
        int len = mem[cur+1] & 0x3fffffff;
        int imm = -((mem[cur+1] >> 31) & 0x1);
        printf("find: cur: %d, next: %d len: %d imm: %d ", cur, next, len, imm);
        printf("name: '");
        for(i=0;i<len;i++) {
            printf("%c",mem[cur+2+i]);
        }
        printf("'\n");
        if(cnt == len) {
            int j;
            int match = 1;
            printf("check match: '");
            for(j=0;j<len;j++) {
                printf("%c", tib[j]);
                if(tib[j] != mem[cur+2+j]) { match = 0; break; } 
            }
            printf("'\n");
            if(match) {
                printf("matched!\n");
                int xt = cur + 1 + len;
                dpush(xt);
                if(imm) {
                    dpush(1);
                } else {
                    dpush(-1);
                }
                return;
            } else {
                printf("no match!\n");
            }
        }
        cur = next;
    }
    // no match
    dpush(in);
    dpush(0);
}

/*
: >digit ( char -- +n true | 0 ) \ "to-digit"
  \ convert char to a digit according to base followed by true, or false if out of range
  DUP [ '9' 1+ ] LITERAL <
  IF '0' - \ convert '0'-'9'
    DUP 0< IF DROP 0 EXIT THEN \ reject < '0'
  ELSE
    BL OR \ convert to lowercase, exploiting ASCII
    'a' -
    DUP 0< IF DROP 0 EXIT THEN \ reject non-letter < 'a'
    #10 + \ convert 'a'-'z'
  THEN
  DUP BASE @ < DUP 0= IF NIP THEN ( +n true | false ) \ reject beyond base
;
*/
//: >NUMBER ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 ) \ "to-number"
//  2SWAP 2>R
//  BEGIN ( c-addr u ) ( R: ud.accum )
//    DUP WHILE \ character left to inspect
//      OVER C@ >digit
//    WHILE \ digit parsed within base
//      2R> BASE @ 1 M*/ ( c-addr u n.digit ud.accum ) \ scale accum by base
//      ROT M+ 2>R \ add current digit to accum
//      1 /STRING ( c-addr1+1 u1-1 )
//  REPEAT THEN
//  2R> 2SWAP ( ud2 c-addr2 u2 )
//;

// ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )
void number() {
    dpop();
}

void interp() {
    while(1) {
        parse_name();
        int cnt = dpop();
        int waddr = dpop();
        dpush(waddr);
        if(cnt == 0) {
            dpop();
            break;
        }
        dpush(cnt);
        find_name();
        int found = dpop();
        printf("found?: %d\n", found);
        if(!found) {
            // try number conversion
            // number();
            // int num_success = dpop();
            int name = dpop();
            int i;
            for(i=0;i<cnt;i++) {
                printf("%c", tib[waddr+i]);
            }
            printf("?\n");
            return;
        }
        // found the word, check state and do stuff
        //if(state) {
            // compile word
        //} else {
            // execute word
        //}
    }
    printf(" ok\n");
}

/*
void interp() {
    while(1) {
        parse_name(); // result: <addr> <cnt>
        find_name(); // result: <addr> <cnt> <lfa>
        int lfa = dpop();
        if(lfa == 0) {
            // not found, try number
            number(); // result: <num> <flag>
            if(!flag) {
                printf("word?\n");
                return;
            }
            if(state) { // compiling
                dpush(LIT1);
                comma();
                comma();
                
            } else { // stack: <num>
                dpush(dpop()); // push number to stack
            }
        } else { // word found
            if(state) { // compiling
                dpush(CALL);
                comma();
                comma();
            } else {
                dpush(lfa);
                execute(); // execute lfa
            }
        }
    }
}
*/

void show_stacks() {
    int i;
    printf("S: ");
    for(i=sp-1;i>=0;i--) {
        printf("%d ", stk[i]);
    }
    printf("\n");
}

int main() {
    here = 130;
    //latest = 130;
    test_create("meow", 0, 0);
    test_create("sandwich", 0, 0);
    while(1) {
        show_stacks();
        accept();
        interp();
        /*
        while(1) {
            if(parse_name() == 0) break;
        }
        */
    }
    return 0;
}

#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 
    fi
}

assert 0 "int main() {0;}"
# assert 42 "42;"
assert 21 "int main() {5+20-4;}"
# assert 41 "12 + 34 - 5;"
# assert 47 "5+6*7;"
# assert 15 "5 * (9 - 6);"
# assert 4 "(3 + 5) / 2;"
assert 15 "int main() {-3 * -5;}"
# assert 15 "-5 * -(2 + 1);"
# assert 9 "5 + +(8 / 2);"
# assert 1 "20 == 4 * 5;"
# assert 1 "27 != 3 * 3 * 2 + 1;"
# assert 0 "57 != 3 * 19;"
# assert 1 "11 * 9 < 10 * 10;"
# assert 1 "20 <= -4 * -5;"
# assert 0 "40 >= 9 * 5;"

# assert 3 "a=3;"
# assert 2 "a=b=2;"
assert 4 "int main() {int a; int b; a=b=2; a+b;}"
# assert 1 "foo=1;"
# assert 6 "foo = 1; bar = 2 + 3; foo + bar;"

assert 5 "int main() {return 5; return 8;}"

# assert 2 "if (1) 2;"
assert 3 "int main() {if (0) 2; else 3;}"

assert 0 "int main() {int i; i = 3; while(i) i = i - 1;}"

# assert 4 "if (0) if (0) 2; else 3; else 4;"

assert 6 "int main() {int sum; int i; sum = 0; for(i = 1; i < 4; i = i + 1) sum = sum + i; sum;}"
assert 4 "int main() {int i; i = 0; for (;;) if (i == 4) return i; else i = i + 1;}"

# assert 3 "{1+2; 2; return 3;}"

# assert 3 "{1; 2; 3;}"
# assert 5 "{foo = 3; bar = 2; return foo + bar;}"
# assert 2 "if (1) {1; 2;}"
# assert 10 "i = 1; sum = 0; while (1) { sum = sum + i; if (i == 4) return sum; i = i + 1;}"

assert 2 "int foo() {return 2;} int main() {foo();}"

assert 5 "int bar(int i, int j) {return i + j;} int main() {bar(2, 3);}"
# assert 21 "goo(1, 2, 3, 4, 5, 6);"

assert 5 "int retfive() {return 5;} int main() {retfive();}"

assert 5 "int bar(int i, int j) {return i + j;} int main() {int i; int j; i = 11; j = 7; return bar(2, 3);}"

assert 3 "int main() {int x; int y; x = 3; y = &x; return *y;}"

echo OK
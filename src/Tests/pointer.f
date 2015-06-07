/* RUN: %clang_cc1 -fsyntax-only -verify -std=c90 -pedantic %s
 */
void
foo (void)
{
 struct b;
 struct b* x = 0;
 struct b* y = &*x;
}

void foo2 (void)
{
 typedef int (*arrayptr)[];
 arrayptr x = 0;
 arrayptr y = &*x;
}

void foo3 (void)
{
 void* x = 0;
 void* y = &*x; /* expected-warning{{address of an expression of type 'void'}} */
}

extern void cv1;

void *foo4 (void)
{
  return &cv1;
}

extern void cv2;
void *foo5 (void)
{
  return &cv2; /* expected-warning{{address of an expression of type 'void'}} */
}

typedef void CVT;
extern CVT cv3;

void *foo6 (void)
{
  extern int aa;
  return &cv3;
}

void main()
{}


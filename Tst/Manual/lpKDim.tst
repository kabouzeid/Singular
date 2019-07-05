LIB "tst.lib"; tst_init();
LIB "fpadim.lib";

ring r = 0,(x,y),dp;
def R = freeAlgebra(r,5);
setring R;
ideal I = x*x, y*y,x*y*x;
ideal J = std(I);
lpKDim(J); // 6

kill r; kill R;

// c_4_1_7_W
ring r = 0,(x4,x3,x2,x1),Dp;
def R = freeAlgebra(r,7);
setring R;
ideal I = x4*x4-25*x4*x2-x1*x4-6*x1*x3-9*x1*x2+x1*x1,
x4*x3+13*x4*x2+12*x4*x1-9*x3*x4+4*x3*x2+41*x3*x1-7*x1*x4-x1*x2,
x3*x3-9*x3*x2+2*x1*x4+x1*x1,
17*x4*x2-5*x2*x2-41*x1*x4,
x2*x2-13*x2*x1-4*x1*x3+2*x1*x2-x1*x1,
x2*x1+4*x1*x2-3*x1*x1;
ideal J = std(I);
lpKDim(J); // 35

kill r; kill R;

// ufn1
ring r = 0,(a,b,c,d),Dp;
def R = freeAlgebra(r,7);
setring R;
ideal I = a*a-a,
b*b-b,
c*c-c,
d*d-d,
a*b*a-a*b,
b*a*b-a*b,
a*c*a-a*c,
c*a*c-a*c,
a*d*a-a*d,
d*a*d-a*d,
b*c*b-b*c,
c*b*c-b*c,
b*d*b-b*d,
d*b*d-b*d,
c*d*c-c*d,
d*c*d-c*d;
ideal J = std(I);
lpKDim(J); // 115

tst_status(1);$

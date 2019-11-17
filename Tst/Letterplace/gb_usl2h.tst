LIB "tst.lib"; tst_init();
LIB "freegb.lib";
ring r = 0,(h,f,e,H),Dp;
int upToDeg = 2;
def R = makeLetterplaceRing(upToDeg);
setring(R);
ideal Id = f*e-e*f+h*H,
h*e-e*h-2*e*H,
h*f-f*h+2*f*H,
f*H-H*f,
e*H-H*e,
h*H-H*h;
std(Id);
tst_status(1);$

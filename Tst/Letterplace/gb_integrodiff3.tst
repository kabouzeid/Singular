LIB "tst.lib"; tst_init();
LIB "freegb.lib";
ring r = 0,(d,x,I),Dp;
int upToDeg = 4;
def R = makeLetterplaceRing(upToDeg);
setring(R);
ideal Id = d*x-x*d-1,
I*x-x*I+I*I,
d*I-1;
std(Id);
tst_status(1);$

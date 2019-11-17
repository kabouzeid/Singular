LIB "tst.lib"; tst_init();
LIB "freegb.lib";
ring r = 0,(x1,x2),Dp;
int upToDeg = 4;
def R = makeLetterplaceRing(upToDeg);
setring(R);
ideal Id = x1*x1*x2,
x1*x2*x2;
std(Id);
tst_status(1);$

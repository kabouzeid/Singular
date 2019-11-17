LIB "tst.lib"; tst_init();
LIB "freegb.lib";
ring r = 0,(s,x),Dp;
int upToDeg = 4;
def R = makeLetterplaceRing(upToDeg);
setring(R);
ideal Id = s*x-x*s-s;
std(Id);
tst_status(1);$

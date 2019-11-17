LIB "tst.lib"; tst_init();
LIB "freegb.lib";
ring r = 0,(y,x),Dp;
int upToDeg = 4;
def R = makeLetterplaceRing(upToDeg);
setring(R);
ideal Id = y*x-x*y,
x*x,
y*y;
std(Id);
tst_status(1);$

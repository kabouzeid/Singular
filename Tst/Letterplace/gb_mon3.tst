LIB "tst.lib"; tst_init();
LIB "freegb.lib";
ring r = 0,(x,y),Dp;
int upToDeg = 4;
def R = makeLetterplaceRing(upToDeg);
setring(R);
ideal Id = x*x,
y*y,
x*y*x,
y*x*y;
std(Id);
tst_status(1);$

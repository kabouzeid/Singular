LIB "tst.lib"; tst_init();
LIB "freegb.lib";
ring r = 0,(d,x,t),Dp;
int upToDeg = 4;
def R = makeLetterplaceRing(upToDeg);
setring(R);
ideal Id = d*x-x*d-d,
t*x-1,
x*t-1;
std(Id);
tst_status(1);$

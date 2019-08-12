#include "Singular/libsingular.h"
#include <vector>

#ifdef HAVE_SHIFTBBA
static BOOLEAN freeAlgebra(leftv res, leftv args)
{
  const short t1[]={2,RING_CMD,INT_CMD};
  const short t2[]={3,RING_CMD,INT_CMD,INT_CMD};
  if (iiCheckTypes(args, t2, 0) || iiCheckTypes(args, t1, 1))
  {
    ring r=(ring)args->Data();
    int d=(int)(long)args->next->Data();
    if (d<2)
    {
      WerrorS("degree must be >=2");
      return TRUE;
    }
    int i=0;
    while(r->order[i]!=0)
    {
      if ((r->order[i]==ringorder_c) ||(r->order[i]==ringorder_C)) i++;
      else if ((r->block0[i]==1)&&(r->block1[i]==r->N)) i++;
      else
      {
        WerrorS("only for rings with a global ordering of one block");
        return TRUE;
      }
    }
    if ((r->order[i]!=0)
    || (rHasLocalOrMixedOrdering(r)))
    {
      WerrorS("only for rings with a global ordering of one block");
      //Werror("only for rings with a global ordering of one block,i=%d, o=%d",i,r->order[i]);
      return TRUE;
    }
    int ncGenCount = 0; 
    if (iiCheckTypes(args,t2,0))
      ncGenCount = (int)(long) args->next->next->Data();
    ring R=freeAlgebra(r,d,ncGenCount);
    res->rtyp=RING_CMD;
    res->data=R;
    return R==NULL;
  }
  return TRUE;
}

static BOOLEAN stest(leftv res, leftv args)
{
  const short t[]={2,POLY_CMD,INT_CMD};
  if (iiCheckTypes(args,t,1))
  {
    poly p=(poly)args->CopyD();
    args=args->next;
    int sh=(int)((long)(args->Data()));
    if (sh<0)
    {
      WerrorS("negative shift for pLPshift");
      return TRUE;
    }
    int L = pLastVblock(p);
    if (L+sh > currRing->N/currRing->isLPring)
    {
      WerrorS("pLPshift: too big shift requested\n");
      return TRUE;
    }
    p_LPshift(p,sh,currRing);
    res->data = p;
    res->rtyp = POLY_CMD;
    return FALSE;
  }
  else return TRUE;
}

static BOOLEAN btest(leftv res, leftv h)
{
  const short t[]={1,POLY_CMD};
  if (iiCheckTypes(h,t,1))
  {
    poly p=(poly)h->Data();
    res->rtyp = INT_CMD;
    res->data = (void*)(long)pLastVblock(p);
    return FALSE;
  }
  else return TRUE;
}

static BOOLEAN p_LPDivisibleBy(ideal I, poly p, ring r)
{
  for(int i = 0; i < IDELEMS(I); i++)
  {
    if (p_LPDivisibleBy(I->m[i], p, r))
    {
      return TRUE;
    }
  }
  return FALSE;
}

static BOOLEAN lpLmDivides(leftv res, leftv h)
{
  const short t1[]={2,POLY_CMD,POLY_CMD};
  const short t2[]={2,IDEAL_CMD,POLY_CMD};
  if (iiCheckTypes(h,t1,0))
  {
    poly p=(poly)h->Data();
    poly q=(poly)h->next->Data();
    res->rtyp = INT_CMD;
    res->data = (void*)(long)p_LPDivisibleBy(p, q, currRing);
    return FALSE;
  }
  else if (iiCheckTypes(h,t2,1))
  {
    ideal I=(ideal)h->Data();
    poly q=(poly)h->next->Data();
    res->rtyp = INT_CMD;
    res->data=(void*)(long) p_LPDivisibleBy(I, q, currRing);
    return FALSE;
  }
  else return TRUE;
}

static BOOLEAN lpVarAt(leftv res, leftv h)
{
  const short t[]={2,POLY_CMD,INT_CMD};
  if (iiCheckTypes(h,t,1))
  {
    poly p=(poly)h->Data();
    int pos=(int)((long)(h->next->Data()));
    res->rtyp = POLY_CMD;
    res->data = p_LPVarAt(p, pos, currRing);
    return FALSE;
  }
  else return TRUE;
}

// returns:
// ATTENTION:
//  - words contains the words normal modulo M of length n
//  - numberOfNormalWords contains the number of words normal modulo M of length 0 ... n
static void _computeNormalWords(ideal words, int& numberOfNormalWords, int n, ideal M, int minDeg, int& last)
{
  if (n <= 0){
    poly one = pOne();
    if (p_LPDivisibleBy(M, one, currRing)) // 1 \in M => no normal words at all
    {
      pDelete(&one);
      last = -1;
      numberOfNormalWords = 0;
    }
    else
    {
      words->m[0] = one;
      last = 0;
      numberOfNormalWords = 1;
    }
    return;
  }

  _computeNormalWords(words, numberOfNormalWords, n - 1, M, minDeg, last);

  int nVars = currRing->isLPring;
  int numberOfNewNormalWords = 0;

  for (int j = nVars - 1; j >= 0; j--)
  {
    for (int i = last; i >= 0; i--)
    {
      int index = (j * (last + 1)) + i;

      if (words->m[i] != NULL)
      {
        if (j > 0) {
          words->m[index] = pCopy(words->m[i]);
        }

        int varOffset = ((n - 1) * nVars) + 1;
        pSetExp(words->m[index], varOffset + j, 1);
        pSetm(words->m[index]);
        pTest(words->m[index]);

        if (n >= minDeg && p_LPDivisibleBy(M, words->m[index], currRing))
        {
          pDelete(&words->m[index]);
          words->m[index] = NULL;
        }
        else
        {
          numberOfNewNormalWords++;
        }
      }
    }
  }

  last = nVars * last + nVars - 1;

  numberOfNormalWords += numberOfNewNormalWords;
}

static ideal computeNormalWords(int length, ideal M)
{
  long minDeg = IDELEMS(M) > 0 ? pTotaldegree(M->m[0]) : 0;
  for (int i = 1; i < IDELEMS(M); i++)
  {
    minDeg = si_min(minDeg, pTotaldegree(M->m[i]));
  }

  int nVars = currRing->isLPring;

  int maxElems = 1;
  for (int i = 0; i < length; i++) // maxElems = nVars^n
    maxElems *= nVars;
  ideal words = idInit(maxElems);
  int last, numberOfNormalWords;
  _computeNormalWords(words, numberOfNormalWords, length, M, minDeg, last);
  idSkipZeroes(words);
  return words;
}

static int countNormalWords(int upToLength, ideal M)
{
  long minDeg = IDELEMS(M) > 0 ? pTotaldegree(M->m[0]) : 0;
  for (int i = 1; i < IDELEMS(M); i++)
  {
    minDeg = si_min(minDeg, pTotaldegree(M->m[i]));
  }

  int nVars = currRing->isLPring;

  int maxElems = 1;
  for (int i = 0; i < upToLength; i++) // maxElems = nVars^n
    maxElems *= nVars;
  ideal words = idInit(maxElems);
  int last, numberOfNormalWords;
  _computeNormalWords(words, numberOfNormalWords, upToLength, M, minDeg, last);
  idDelete(&words);
  return numberOfNormalWords;
}

// NULL if graph is undefined
static intvec* ufnarovskiGraph(ideal G)
{
  long l = 0;
  for (int i = 0; i < IDELEMS(G); i++)
    l = si_max(pTotaldegree(G->m[i]), l);
  l--;
  if (l <= 0)
  {
    WerrorS("Ufnarovski graph not implemented for l <= 0");
    return NULL;
  }
  int lV = currRing->isLPring;

  ideal normalWords = computeNormalWords(l, G);

  int n = IDELEMS(normalWords);
  intvec* UG = new intvec(n, n, 0);
  for (int i = 0; i < n; i++)
  {
    for (int j = 0; j < n; j++)
    {
      poly v = normalWords->m[i];
      poly w = normalWords->m[j];

      // check whether v*x1 = x2*w (overlap)
      bool overlap = true;
      for (int k = 1; k <= (l - 1) * lV; k++)
      {
        if (pGetExp(v, k + lV) != pGetExp(w, k)) {
          overlap = false;
          break;
        }
      }

      if (overlap)
      {
        // create the overlap
        poly p = pMult(pCopy(v), p_LPVarAt(w, l, currRing));

        // check whether the overlap is normal
        bool normal = true;
        for (int k = 0; k < IDELEMS(G); k++)
        {
          if (p_LPDivisibleBy(G->m[k], p, currRing))
          {
            normal = false;
            break;
          }
        }

        if (normal)
        {
          IMATELEM(*UG, i + 1, j + 1) = 1;
        }
      }
    }
  }
  return UG;
}

static std::vector<int> countCycles(const intvec* _G, int v, std::vector<int> path, std::vector<BOOLEAN> visited, std::vector<BOOLEAN> cyclic, std::vector<int> cache)
{
  intvec* G = ivCopy(_G); // modifications must be local

  if (cache[v] != -2) return cache; // value is already cached

  visited[v] = TRUE;
  path.push_back(v);

  int cycles = 0;
  for (int w = 0; w < G->cols(); w++)
  {
    if (IMATELEM(*G, v + 1, w + 1)) // edge v -> w exists in G
    {
      if (!visited[w])
      { // continue with w
        cache = countCycles(G, w, path, visited, cyclic, cache);
        if (cache[w] == -1)
        {
          cache[v] = -1;
          return cache;
        }
        cycles = si_max(cycles, cache[w]);
      }
      else
      { // found new cycle
        int pathIndexOfW = -1;
        for (int i = path.size() - 1; i >= 0; i--) {
          if (cyclic[path[i]] == 1) { // found an already cyclic vertex
            cache[v] = -1;
            return cache;
          }
          cyclic[path[i]] = TRUE;

          if (path[i] == w) { // end of the cycle
            assume(IMATELEM(*G, v + 1, w + 1) != 0);
            IMATELEM(*G, v + 1, w + 1) = 0; // remove edge v -> w
            pathIndexOfW = i;
            break;
          } else {
            assume(IMATELEM(*G, path[i - 1] + 1, path[i] + 1) != 0);
            IMATELEM(*G, path[i - 1] + 1, path[i] + 1) = 0; // remove edge vi-1 -> vi
          }
        }
        assume(pathIndexOfW != -1); // should never happen
        for (int i = path.size() - 1; i >= pathIndexOfW; i--) {
          cache = countCycles(G, path[i], path, visited, cyclic, cache);
          if (cache[path[i]] == -1)
          {
            cache[v] = -1;
            return cache;
          }
          cycles = si_max(cycles, cache[path[i]] + 1);
        }
      }
    }
  }
  cache[v] = cycles;

  delete G;
  return cache;
}

// -1 is infinity
static int graphGrowth(const intvec* G)
{
  // init
  int n = G->cols();
  std::vector<int> path;
  std::vector<BOOLEAN> visited;
  std::vector<BOOLEAN> cyclic;
  std::vector<int> cache;
  visited.resize(n, FALSE);
  cyclic.resize(n, FALSE);
  cache.resize(n, -2);

  // get max number of cycles
  int cycles = 0;
  for (int v = 0; v < n; v++)
  {
    cache = countCycles(G, v, path, visited, cyclic, cache);
    if (cache[v] == -1)
      return -1;
    cycles = si_max(cycles, cache[v]);
  }
  return cycles;
}

// -1 is infinity, -2 is error
static int gkDim(const ideal _G)
{
  if (rField_is_Ring(currRing)) {
      WerrorS("GK-Dim not implemented for rings");
      return -2;
  }

  for (int i=IDELEMS(_G)-1;i>=0; i--)
  {
    if (pGetComp(_G->m[i]) != 0)
    {
      WerrorS("GK-Dim not implemented for modules");
      return -2;
    }
  }

  ideal G = id_Head(_G, currRing); // G = LM(G) (and copy)
  idSkipZeroes(G); // remove zeros
  id_DelLmEquals(G, currRing); // remove duplicates

  // get the max deg
  long maxDeg = 0;
  for (int i = 0; i < IDELEMS(G); i++)
  {
    maxDeg = si_max(maxDeg, pTotaldegree(G->m[i]));

    // also check whether G = <1>
    if (pIsConstantComp(G->m[i]))
    {
      WerrorS("GK-Dim not defined for 0-ring");
      return -2;
    }
  }

  // early termination if G \subset X
  if (maxDeg <= 1)
  {
    int lV = currRing->isLPring;
    if (IDELEMS(G) == lV) // V = {1} no edges
      return 0;
    if (IDELEMS(G) == lV - 1) // V = {1} with loop
      return 1;
    if (IDELEMS(G) <= lV - 2) // V = {1} with more than one loop
      return -1;
  }

  intvec* UG = ufnarovskiGraph(G);
  if (errorreported || UG == NULL) return -2;
  return graphGrowth(UG);
}

// converts an intvec matrix to a vector<vector<int>>
static std::vector<std::vector<int>> iv2vv(intvec* M)
{
  int rows = M->rows();
  int cols = M->cols();

  std::vector<std::vector<int>> mat(rows, std::vector<int>(cols));

  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      mat[i][j] = IMATELEM(*M, i + 1, j + 1);
    }
  }

  return mat;
}

static void vvPrint(const std::vector<std::vector<int>>& mat)
{
  for (int i = 0; i < mat.size(); i++)
  {
    for (int j = 0; j < mat[i].size(); j++)
    {
      Print("%d ", mat[i][j]);
    }
    PrintLn();
  }
}

static void vvTest(const std::vector<std::vector<int>>& mat)
{
  if (mat.size() > 0)
  {
    int cols = mat[0].size();
    for (int i = 1; i < mat.size(); i++)
    {
      if (cols != mat[i].size())
        WerrorS("number of cols in matrix inconsistent");
    }
  }
}

static void vvDeleteRow(std::vector<std::vector<int>>& mat, int row)
{
  mat.erase(mat.begin() + row);
}

static void vvDeleteColumn(std::vector<std::vector<int>>& mat, int col)
{
  for (int i = 0; i < mat.size(); i++)
  {
    mat[i].erase(mat[i].begin() + col);
  }
}

static BOOLEAN vvIsRowZero(const std::vector<std::vector<int>>& mat, int row)
{
  for (int i = 0; i < mat[row].size(); i++)
  {
    if (mat[row][i] != 0)
      return FALSE;
  }
  return TRUE;
}

static BOOLEAN vvIsColumnZero(const std::vector<std::vector<int>>& mat, int col)
{
  for (int i = 0; i < mat.size(); i++)
  {
    if (mat[i][col] != 0)
      return FALSE;
  }
  return TRUE;
}

static BOOLEAN vvIsZero(const std::vector<std::vector<int>>& mat)
{
  for (int i = 0; i < mat.size(); i++)
  {
    if (!vvIsRowZero(mat, i))
      return FALSE;
  }
  return TRUE;
}

static std::vector<std::vector<int>> vvMult(const std::vector<std::vector<int>>& a, const std::vector<std::vector<int>>& b)
{
  int ra = a.size();
  int rb = b.size();
  int ca = a.size() > 0 ? a[0].size() : 0;
  int cb = b.size() > 0 ? b[0].size() : 0;

  if (ca != rb)
  {
    WerrorS("matrix dimensions do not match");
    return std::vector<std::vector<int>>();
  }

  std::vector<std::vector<int>> res(ra, std::vector<int>(cb));
  for (int i = 0; i < ra; i++)
  {
    for (int j = 0; j < cb; j++)
    {
      int sum = 0;
      for (int k = 0; k < ca; k++)
        sum += a[i][k] * b[k][j];
      res[i][j] = sum;
    }
  }
  return res;
}

// -1 is infinity, -2 is error
static int kDim(const ideal _G)
{
  if (rField_is_Ring(currRing)) {
      WerrorS("K-Dim not implemented for rings");
      return -2;
  }

  for (int i=IDELEMS(_G)-1;i>=0; i--)
  {
    if (pGetComp(_G->m[i]) != 0)
    {
      WerrorS("K-Dim not implemented for modules");
      return -2;
    }
  }

  ideal G = id_Head(_G, currRing); // G = LM(G) (and copy)
  if (TEST_OPT_PROT)
    Print("%d original generators\n", IDELEMS(G));
  idSkipZeroes(G); // remove zeros
  id_DelLmEquals(G, currRing); // remove duplicates
  if (TEST_OPT_PROT)
    Print("%d non-zero unique generators\n", IDELEMS(G));

  // get the max deg
  long maxDeg = 0;
  for (int i = 0; i < IDELEMS(G); i++)
  {
    maxDeg = si_max(maxDeg, pTotaldegree(G->m[i]));

    // also check whether G = <1>
    if (pIsConstantComp(G->m[i]))
    {
      WerrorS("K-Dim not defined for 0-ring"); // TODO is it minus infinity ?
      return -2;
    }
  }
  if (TEST_OPT_PROT)
    Print("max deg: %ld\n", maxDeg);


  // for normal words of length minDeg ... maxDeg-1
  // brute-force the normal words
  if (TEST_OPT_PROT)
    Print("computing normal words normally\n");
  long numberOfNormalWords = countNormalWords(maxDeg - 1, G);

  if (TEST_OPT_PROT)
    Print("%ld normal words up to length %ld\n", numberOfNormalWords, maxDeg - 1);

  int lV = currRing->isLPring; // |X|
  // early termination if G \subset X
  if (maxDeg <= 1)
  {
    if (IDELEMS(G) == lV) // V = {1} no edges
      return numberOfNormalWords;
    if (IDELEMS(G) == lV - 1) // V = {1} with loop
      return -1;
    if (IDELEMS(G) <= lV - 2) // V = {1} with more than one loop
      return -1;
  }

  if (TEST_OPT_PROT)
    Print("computing Ufnarovski graph\n");

  intvec* UG = ufnarovskiGraph(G);
  if (errorreported || UG == NULL) return -2;

  if (TEST_OPT_PROT)
    Print("Ufnarovski graph is %dx%d \n", UG->rows(), UG->cols());
  std::vector<std::vector<int>> vvUG = iv2vv(UG);
  for (int i = 0; i < vvUG.size(); i++)
  {
    if (vvIsRowZero(vvUG, i) && vvIsColumnZero(vvUG, i)) // i is isolated vertex
    {
      vvDeleteRow(vvUG, i);
      vvDeleteColumn(vvUG, i);
      i--;
    }
  }
  if (TEST_OPT_PROT)
    Print("Simplified Ufnarovski graph to %ldx%ld\n", vvUG.size(), vvUG.size());

  // for normal words of length >= maxDeg
  // use Ufnarovski graph
  if (TEST_OPT_PROT)
    Print("computing normal words via Ufnarovski graph\n");
  std::vector<std::vector<int>> UGpower = vvUG;
  long nUGpower = 1;
  while (!vvIsZero(UGpower))
  {
    if (TEST_OPT_PROT)
      Print("Start count graph entries\n");
    for (int i = 0; i < UGpower.size(); i++)
    {
      for (int j = 0; j < UGpower[i].size(); j++)
      {
        numberOfNormalWords += UGpower[i][j];
      }
    }

    if (TEST_OPT_PROT)
    {
      Print("Done count graph entries\n");
      Print("%ld normal words up to length %ld\n", numberOfNormalWords, maxDeg - 1 + nUGpower);
    }

    if (TEST_OPT_PROT)
      Print("Start mat mult\n");
    UGpower = vvMult(UGpower, vvUG); // TODO avoid creation of new intvec
    if (TEST_OPT_PROT)
      Print("Done mat mult\n");
    nUGpower++;
  }

  // idDelete(&G); // TODO delete?

  return numberOfNormalWords;
}

static BOOLEAN lpGkDim(leftv res, leftv h)
{
  const short t[]={1,IDEAL_CMD};
  if (iiCheckTypes(h,t,1))
  {
    assumeStdFlag(h);
    ideal G = (ideal) h->Data();
    res->rtyp = INT_CMD;
    res->data = (void*)(long) gkDim(G);
    if (errorreported) return TRUE;
    return FALSE;
  }
  else return TRUE;
}

static BOOLEAN lpKDim(leftv res, leftv h)
{
  const short t[]={1,IDEAL_CMD};
  if (iiCheckTypes(h,t,1))
  {
    assumeStdFlag(h);
    ideal G = (ideal) h->Data();
    res->rtyp = INT_CMD;
    res->data = (void*)(long) kDim(G);
    if (errorreported) return TRUE;
    return FALSE;
  }
  else return TRUE;
}

static BOOLEAN lpUfnarovskiGraph(leftv res, leftv h)
{
  const short t[]={1,IDEAL_CMD};
  if (iiCheckTypes(h,t,1))
  {
    assumeStdFlag(h);
    ideal G = (ideal) h->Data();
    res->rtyp = INTVEC_CMD;
    res->data = ufnarovskiGraph(G);
    if (errorreported) return TRUE;
    return FALSE;
  }
  else return TRUE;
}
#endif

//------------------------------------------------------------------------
// initialisation of the module
extern "C" int SI_MOD_INIT(freealgebra)(SModulFunctions* p)
{
#ifdef HAVE_SHIFTBBA
  p->iiAddCproc("freealgebra.so","freeAlgebra",FALSE,freeAlgebra);
  p->iiAddCproc("freealgebra.so","lpLmDivides",FALSE,lpLmDivides);
  p->iiAddCproc("freealgebra.so","lpVarAt",FALSE,lpVarAt);
  p->iiAddCproc("freealgebra.so","lpGkDim",FALSE,lpGkDim);
  p->iiAddCproc("freealgebra.so","lpKDim",FALSE,lpKDim);
  p->iiAddCproc("freealgebra.so","lpUfnarovskiGraph",FALSE,lpUfnarovskiGraph);

  // library private methods
  p->iiAddCproc("freealgebra.so","stest",TRUE,stest);
  p->iiAddCproc("freealgebra.so","btest",TRUE,btest);
#endif
  return (MAX_TOK);
}

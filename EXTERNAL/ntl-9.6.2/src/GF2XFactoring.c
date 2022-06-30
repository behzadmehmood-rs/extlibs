
#include <NTL/GF2XFactoring.h>

#include <NTL/new.h>

NTL_START_IMPL


long IterIrredTest(const GF2X& f)
{
   long df = deg(f);

   if (df <= 0) return 0;
   if (df == 1) return 1;

   GF2XModulus F;

   build(F, f);
   
   GF2X h;
   SetX(h);
   SqrMod(h, h, F);

   long i, d, limit, limit_sqr;
   GF2X g, X, t, prod;


   SetX(X);

   i = 0;
   g = h;
   d = 1;
   limit = 2;
   limit_sqr = limit*limit;

   set(prod);

   while (2*d <= df) {
      add(t, g, X);
      MulMod(prod, prod, t, F);
      i++;
      if (i == limit_sqr) {
         GCD(t, f, prod);
         if (!IsOne(t)) return 0;

         set(prod);
         limit++;
         limit_sqr = limit*limit;
         i = 0;
      }

      d = d + 1;
      if (2*d <= deg(f)) {
         SqrMod(g, g, F);
      }
   }

   if (i > 0) {
      GCD(t, f, prod);
      if (!IsOne(t)) return 0;
   }

   return 1;
}


void SquareFreeDecomp(vec_pair_GF2X_long& u, const GF2X& ff)
{
   GF2X f = ff;

   if (IsZero(f)) LogicError("SquareFreeDecomp: bad args");

   GF2X r, t, v, tmp1;
   long m, j, finished, done;

   u.SetLength(0);

   if (deg(f) == 0)
      return;

   m = 1;
   finished = 0;

   do {
      j = 1;
      diff(tmp1, f);
      GCD(r, f, tmp1);
      div(t, f, r);

      if (deg(t) > 0) {
         done = 0;
         do {
            GCD(v, r, t);
            div(tmp1, t, v);
            if (deg(tmp1) > 0) append(u, cons(tmp1, j*m));
            if (deg(v) > 0) {
               div(r, r, v);
               t = v;
               j++;
            }
            else
               done = 1;
         } while (!done);
         if (deg(r) == 0) finished = 1;
      }

      if (!finished) {
         /* r is a p-th power */
         long p, k, d;
         p = 2;
         d = deg(r)/p;
         clear(f);
         for (k = 0; k <= d; k++)
            if (coeff(r, k*p) == 1)
               SetCoeff(f, k);

         m = m*p;
      }
   } while (!finished);
}
         



static
void AddFactor(vec_pair_GF2X_long& factors, const GF2X& g, long d, long verbose)
{
   if (verbose)
      cerr << "degree=" << d << ", number=" << deg(g)/d << "\n";
   append(factors, cons(g, d));
}

static
void ProcessTable(GF2X& f, vec_pair_GF2X_long& factors, 
                  const GF2XModulus& F, long limit, const vec_GF2X& tbl,
                  long d, long verbose)

{
   if (limit == 0) return;

   if (verbose) cerr << "+";

   GF2X t1;

   if (limit == 1) {
      GCD(t1, f, tbl[0]);
      if (deg(t1) > 0) {
         AddFactor(factors, t1, d, verbose);
         div(f, f, t1);
      }

      return;
   }

   long i;

   t1 = tbl[0];
   for (i = 1; i < limit; i++)
      MulMod(t1, t1, tbl[i], F);

   GCD(t1, f, t1);

   if (deg(t1) == 0) return;

   div(f, f, t1);

   GF2X t2;

   i = 0;
   d = d - limit + 1;

   while (2*d <= deg(t1)) {
      GCD(t2, tbl[i], t1); 
      if (deg(t2) > 0) {
         AddFactor(factors, t2, d, verbose);
         div(t1, t1, t2);
      }

      i++;
      d++;
   }

   if (deg(t1) > 0)
      AddFactor(factors, t1, deg(t1), verbose);
}


static
void TraceMap(GF2X& w, const GF2X& a, long d, const GF2XModulus& F)

{
   GF2X y, z;

   y = a;
   z = a;

   long i;

   for (i = 1; i < d; i++) {
      SqrMod(z, z, F);
      add(y, y, z);
   }

   w = y;
}


const long GF2X_BlockingFactor = 40;

void DDF(vec_pair_GF2X_long& factors, const GF2X& ff, long verbose)
{
   GF2X f = ff;

   if (IsZero(f)) LogicError("DDF: bad args");

   factors.SetLength(0);

   if (deg(f) == 0)
      return;

   if (deg(f) == 1) {
      AddFactor(factors, f, 1, verbose);
      return;
   }


   long GCDTableSize = GF2X_BlockingFactor;

   GF2XModulus F;
   build(F, f);

   long i, d, limit, old_n;
   GF2X g, X;


   vec_GF2X tbl(INIT_SIZE, GCDTableSize);

   SetX(X);

   i = 0;
   SqrMod(g, X, F);
   d = 1;
   limit = GCDTableSize;


   while (2*d <= deg(f)) {

      old_n = deg(f);
      add(tbl[i], g, X);
      i++;
      if (i == limit) {
         ProcessTable(f, factors, F, i, tbl, d, verbose);
         i = 0;
      }

      d = d + 1;
      if (2*d <= deg(f)) {
         // we need to go further

         if (deg(f) < old_n) {
            // f has changed 

            build(F, f);
            rem(g, g, F);
         }

         SqrMod(g, g, F);
      }
   }

   ProcessTable(f, factors, F, i, tbl, d-1, verbose);

   if (!IsOne(f)) AddFactor(factors, f, deg(f), verbose);
}




static
void EDFSplit(GF2X& f1, GF2X& f2, const GF2X& f, long d)
{
   GF2X a, g;
   GF2XModulus F;
   
   build(F, f);
   long n = F.n;

   do {
      random(a, n);
      TraceMap(g, a, d, F);
   } while (deg(g) <= 0);

   GCD(f1, f, g);
   div(f2, f, f1);
}

static
void RecEDF(vec_GF2X& factors, const GF2X& f, long d)
{
   if (deg(f) == d) {
      append(factors, f);
      return;
   }

   GF2X f1, f2;

   EDFSplit(f1, f2, f, d);
   RecEDF(factors, f1, d);
   RecEDF(factors, f2, d);
}
         

void EDF(vec_GF2X& factors, const GF2X& ff, long d, long verbose)

{
   GF2X f = ff;

   if (IsZero(f)) LogicError("EDF: bad args");

   long n = deg(f);
   long r = n/d;

   if (r == 0) {
      factors.SetLength(0);
      return;
   }

   if (r == 1) {
      factors.SetLength(1);
      factors[0] = f;
      return;
   }

   if (d == 1) {
      // factors are X and X+1

      factors.SetLength(2);
      SetX(factors[0]);
      SetX(factors[1]);
      SetCoeff(factors[1], 0);
      return;
   }

   
   double t;
   if (verbose) { 
      cerr << "computing EDF(" << d << "," << r << ")..."; 
      t = GetTime(); 
   }

   factors.SetLength(0);

   RecEDF(factors, f, d);

   if (verbose) cerr << (GetTime()-t) << "\n";
}


void SFCanZass(vec_GF2X& factors, const GF2X& ff, long verbose)
{
   GF2X f = ff;

   if (IsZero(f)) LogicError("SFCanZass: bad args");

   if (deg(f) == 0) {
      factors.SetLength(0);
      return;
   }

   if (deg(f) == 1) {
      factors.SetLength(1);
      factors[0] = f;
      return;
   }

   factors.SetLength(0);

   double t;

   
   vec_pair_GF2X_long u;
   if (verbose) { cerr << "computing DDF..."; t = GetTime(); }
   DDF(u, f, verbose);
   if (verbose) { 
      t = GetTime()-t; 
      cerr << "DDF time: " << t << "\n";
   }

   vec_GF2X v;

   long i;
   for (i = 0; i < u.length(); i++) {
      const GF2X& g = u[i].a;
      long d = u[i].b;
      long r = deg(g)/d;

      if (r == 1) {
         // g is already irreducible

         append(factors, g);
      }
      else {
         // must perform EDF

         EDF(v, g, d, verbose);
         append(factors, v);
      }
   }
}
   
void CanZass(vec_pair_GF2X_long& factors, const GF2X& f, long verbose)
{
   if (IsZero(f))
      LogicError("CanZass: bad args");

   double t;
   vec_pair_GF2X_long sfd;
   vec_GF2X x;

   
   if (verbose) { cerr << "square-free decomposition..."; t = GetTime(); }
   SquareFreeDecomp(sfd, f);
   if (verbose) cerr << (GetTime()-t) << "\n";

   factors.SetLength(0);

   long i, j;

   for (i = 0; i < sfd.length(); i++) {
      if (verbose) {
         cerr << "factoring multiplicity " << sfd[i].b 
              << ", deg = " << deg(sfd[i].a) << "\n";
      }

      SFCanZass(x, sfd[i].a, verbose);

      for (j = 0; j < x.length(); j++)
         append(factors, cons(x[j], sfd[i].b));
   }
}

void mul(GF2X& f, const vec_pair_GF2X_long& v)
{
   long i, j, n;

   n = 0;
   for (i = 0; i < v.length(); i++)
      n += v[i].b*deg(v[i].a);

   GF2X g;

   set(g);
   for (i = 0; i < v.length(); i++)
      for (j = 0; j < v[i].b; j++) {
         mul(g, g, v[i].a);
      }

   f = g;
}



static
void ConvertBits(GF2X& x, _ntl_ulong b)
{
   clear(x);
   long i;

   for (i = NTL_BITS_PER_LONG-1; i >= 0; i--)
      if (b & (1UL << i))
         SetCoeff(x, i);

}

void BuildIrred(GF2X& f, long n)
{
   if (n <= 0)
      LogicError("BuildIrred: n must be positive");

   if (NTL_OVERFLOW(n, 1, 0)) ResourceError("overflow in BuildIrred");

   if (n == 1) {
      SetX(f);
      return;
   }

   GF2X g;

   _ntl_ulong i;

   i = 0;
   do {
      ConvertBits(g, 2*i+1);
      SetCoeff(g, n);
      i++;
   } while (!IterIrredTest(g));

   f = g;

}

void BuildRandomIrred(GF2X& f, const GF2X& g)
{
   GF2XModulus G;
   GF2X h, ff;

   build(G, g);
   do {
      random(h, deg(g));
      IrredPolyMod(ff, h, G);
   } while (deg(ff) < deg(g));

   f = ff;
}




static int GF2X_irred_tab[][3] = 

{{0,0,0}, {0,0,0},
{1,0,0}, {1,0,0}, {1,0,0}, {2,0,0}, {1,0,0}, {1,0,0}, 
{4,3,1}, {1,0,0}, {3,0,0}, {2,0,0}, {3,0,0}, {4,3,1}, 
{5,0,0}, {1,0,0}, {5,3,1}, {3,0,0}, {3,0,0}, {5,2,1}, 
{3,0,0}, {2,0,0}, {1,0,0}, {5,0,0}, {4,3,1}, {3,0,0}, 
{4,3,1}, {5,2,1}, {1,0,0}, {2,0,0}, {1,0,0}, {3,0,0}, 
{7,3,2}, {10,0,0}, {7,0,0}, {2,0,0}, {9,0,0}, {6,4,1}, 
{6,5,1}, {4,0,0}, {5,4,3}, {3,0,0}, {7,0,0}, {6,4,3}, 
{5,0,0}, {4,3,1}, {1,0,0}, {5,0,0}, {5,3,2}, {9,0,0}, 
{4,3,2}, {6,3,1}, {3,0,0}, {6,2,1}, {9,0,0}, {7,0,0}, 
{7,4,2}, {4,0,0}, {19,0,0}, {7,4,2}, {1,0,0}, {5,2,1}, 
{29,0,0}, {1,0,0}, {4,3,1}, {18,0,0}, {3,0,0}, {5,2,1}, 
{9,0,0}, {6,5,2}, {5,3,1}, {6,0,0}, {10,9,3}, {25,0,0}, 
{35,0,0}, {6,3,1}, {21,0,0}, {6,5,2}, {6,5,3}, {9,0,0}, 
{9,4,2}, {4,0,0}, {8,3,1}, {7,4,2}, {5,0,0}, {8,2,1}, 
{21,0,0}, {13,0,0}, {7,6,2}, {38,0,0}, {27,0,0}, {8,5,1}, 
{21,0,0}, {2,0,0}, {21,0,0}, {11,0,0}, {10,9,6}, {6,0,0}, 
{11,0,0}, {6,3,1}, {15,0,0}, {7,6,1}, {29,0,0}, {9,0,0}, 
{4,3,1}, {4,0,0}, {15,0,0}, {9,7,4}, {17,0,0}, {5,4,2}, 
{33,0,0}, {10,0,0}, {5,4,3}, {9,0,0}, {5,3,2}, {8,7,5}, 
{4,2,1}, {5,2,1}, {33,0,0}, {8,0,0}, {4,3,1}, {18,0,0}, 
{6,2,1}, {2,0,0}, {19,0,0}, {7,6,5}, {21,0,0}, {1,0,0}, 
{7,2,1}, {5,0,0}, {3,0,0}, {8,3,2}, {17,0,0}, {9,8,2}, 
{57,0,0}, {11,0,0}, {5,3,2}, {21,0,0}, {8,7,1}, {8,5,3}, 
{15,0,0}, {10,4,1}, {21,0,0}, {5,3,2}, {7,4,2}, {52,0,0}, 
{71,0,0}, {14,0,0}, {27,0,0}, {10,9,7}, {53,0,0}, {3,0,0}, 
{6,3,2}, {1,0,0}, {15,0,0}, {62,0,0}, {9,0,0}, {6,5,2}, 
{8,6,5}, {31,0,0}, {5,3,2}, {18,0,0}, {27,0,0}, {7,6,3}, 
{10,8,7}, {9,8,3}, {37,0,0}, {6,0,0}, {15,3,2}, {34,0,0}, 
{11,0,0}, {6,5,2}, {1,0,0}, {8,5,2}, {13,0,0}, {6,0,0}, 
{11,3,2}, {8,0,0}, {31,0,0}, {4,2,1}, {3,0,0}, {7,6,1}, 
{81,0,0}, {56,0,0}, {9,8,7}, {24,0,0}, {11,0,0}, {7,6,5}, 
{6,5,2}, {6,5,2}, {8,7,6}, {9,0,0}, {7,2,1}, {15,0,0}, 
{87,0,0}, {8,3,2}, {3,0,0}, {9,4,2}, {9,0,0}, {34,0,0}, 
{5,3,2}, {14,0,0}, {55,0,0}, {8,7,1}, {27,0,0}, {9,5,2}, 
{10,9,5}, {43,0,0}, {9,3,1}, {6,0,0}, {7,0,0}, {11,10,8}, 
{105,0,0}, {6,5,2}, {73,0,0}, {23,0,0}, {7,3,1}, {45,0,0}, 
{11,0,0}, {8,4,1}, {7,0,0}, {8,6,2}, {5,4,2}, {33,0,0}, 
{9,8,3}, {32,0,0}, {10,7,3}, {10,9,4}, {113,0,0}, {10,4,1}, 
{8,7,6}, {26,0,0}, {9,4,2}, {74,0,0}, {31,0,0}, {9,6,1}, 
{5,0,0}, {7,4,1}, {73,0,0}, {36,0,0}, {8,5,3}, {70,0,0}, 
{95,0,0}, {8,5,1}, {111,0,0}, {6,4,1}, {11,2,1}, {82,0,0}, 
{15,14,10}, {35,0,0}, {103,0,0}, {7,4,2}, {15,0,0}, {46,0,0}, 
{7,2,1}, {52,0,0}, {10,5,2}, {12,0,0}, {71,0,0}, {10,6,2}, 
{15,0,0}, {7,6,4}, {9,8,4}, {93,0,0}, {9,6,2}, {42,0,0}, 
{47,0,0}, {8,6,3}, {25,0,0}, {7,6,1}, {53,0,0}, {58,0,0}, 
{9,3,2}, {23,0,0}, {67,0,0}, {11,10,9}, {63,0,0}, {12,6,3}, 
{5,0,0}, {5,0,0}, {9,5,2}, {93,0,0}, {35,0,0}, {12,7,5}, 
{53,0,0}, {10,7,5}, {69,0,0}, {71,0,0}, {11,10,1}, {21,0,0}, 
{5,3,2}, {12,11,5}, {37,0,0}, {11,6,1}, {33,0,0}, {48,0,0}, 
{7,3,2}, {5,0,0}, {11,8,4}, {11,6,4}, {5,0,0}, {9,5,2}, 
{41,0,0}, {1,0,0}, {11,2,1}, {102,0,0}, {7,3,1}, {8,4,2}, 
{15,0,0}, {10,6,4}, {93,0,0}, {7,5,3}, {9,7,4}, {79,0,0}, 
{15,0,0}, {10,9,1}, {63,0,0}, {7,4,2}, {45,0,0}, {36,0,0}, 
{4,3,1}, {31,0,0}, {67,0,0}, {10,3,1}, {51,0,0}, {10,5,2}, 
{10,3,1}, {34,0,0}, {8,3,1}, {50,0,0}, {99,0,0}, {10,6,2}, 
{89,0,0}, {2,0,0}, {5,2,1}, {10,7,2}, {7,4,1}, {55,0,0}, 
{4,3,1}, {16,10,7}, {45,0,0}, {10,8,6}, {125,0,0}, {75,0,0}, 
{7,2,1}, {22,0,0}, {63,0,0}, {11,10,3}, {103,0,0}, {6,5,2}, 
{53,0,0}, {34,0,0}, {13,11,6}, {69,0,0}, {99,0,0}, {6,5,1}, 
{10,9,7}, {11,10,2}, {57,0,0}, {68,0,0}, {5,3,2}, {7,4,1}, 
{63,0,0}, {8,5,3}, {9,0,0}, {9,6,5}, {29,0,0}, {21,0,0}, 
{7,3,2}, {91,0,0}, {139,0,0}, {8,3,2}, {111,0,0}, {8,7,2}, 
{8,6,5}, {16,0,0}, {8,7,5}, {41,0,0}, {43,0,0}, {10,8,5}, 
{47,0,0}, {5,2,1}, {81,0,0}, {90,0,0}, {12,3,2}, {6,0,0}, 
{83,0,0}, {8,7,1}, {159,0,0}, {10,9,5}, {9,0,0}, {28,0,0}, 
{13,10,6}, {7,0,0}, {135,0,0}, {11,6,5}, {25,0,0}, {12,7,6}, 
{7,6,2}, {26,0,0}, {5,3,2}, {152,0,0}, {171,0,0}, {9,8,5}, 
{65,0,0}, {13,8,2}, {141,0,0}, {71,0,0}, {5,3,2}, {87,0,0}, 
{10,4,3}, {12,10,3}, {147,0,0}, {10,7,6}, {13,0,0}, {102,0,0}, 
{9,5,2}, {107,0,0}, {199,0,0}, {15,5,4}, {7,0,0}, {5,4,2}, 
{149,0,0}, {25,0,0}, {9,7,2}, {12,0,0}, {63,0,0}, {11,6,5}, 
{105,0,0}, {10,8,7}, {14,6,1}, {120,0,0}, {13,4,3}, {33,0,0}, 
{12,11,5}, {12,9,5}, {165,0,0}, {6,2,1}, {65,0,0}, {49,0,0}, 
{4,3,1}, {7,0,0}, {7,5,2}, {10,6,1}, {81,0,0}, {7,6,4}, 
{105,0,0}, {73,0,0}, {11,6,4}, {134,0,0}, {47,0,0}, {16,10,1}, 
{6,5,4}, {15,6,4}, {8,6,1}, {38,0,0}, {18,9,6}, {16,0,0}, 
{203,0,0}, {12,5,2}, {19,0,0}, {7,6,1}, {73,0,0}, {93,0,0}, 
{19,18,13}, {31,0,0}, {14,11,6}, {11,6,1}, {27,0,0}, {9,5,2}, 
{9,0,0}, {1,0,0}, {11,3,2}, {200,0,0}, {191,0,0}, {9,8,4}, 
{9,0,0}, {16,15,7}, {121,0,0}, {104,0,0}, {15,9,6}, {138,0,0}, 
{9,6,5}, {9,6,4}, {105,0,0}, {17,16,6}, {81,0,0}, {94,0,0}, 
{4,3,1}, {83,0,0}, {219,0,0}, {11,6,3}, {7,0,0}, {10,5,3}, 
{17,0,0}, {76,0,0}, {16,5,2}, {78,0,0}, {155,0,0}, {11,6,5}, 
{27,0,0}, {5,4,2}, {8,5,4}, {3,0,0}, {15,14,6}, {156,0,0}, 
{23,0,0}, {13,6,3}, {9,0,0}, {8,7,3}, {69,0,0}, {10,0,0}, 
{8,5,2}, {26,0,0}, {67,0,0}, {14,7,4}, {21,0,0}, {12,10,2}, 
{33,0,0}, {79,0,0}, {15,11,2}, {32,0,0}, {39,0,0}, {13,6,2}, 
{167,0,0}, {6,4,1}, {97,0,0}, {47,0,0}, {11,6,2}, {42,0,0}, 
{10,7,3}, {10,5,4}, {1,0,0}, {4,3,2}, {161,0,0}, {8,6,2}, 
{7,5,3}, {94,0,0}, {195,0,0}, {10,5,4}, {9,0,0}, {13,10,4}, 
{8,6,1}, {16,0,0}, {8,3,1}, {122,0,0}, {8,2,1}, {13,7,4}, 
{10,5,3}, {16,4,3}, {193,0,0}, {135,0,0}, {19,16,9}, {39,0,0}, 
{10,8,7}, {10,9,4}, {153,0,0}, {7,6,5}, {73,0,0}, {34,0,0}, 
{11,9,6}, {71,0,0}, {11,4,2}, {14,7,3}, {163,0,0}, {11,6,1}, 
{153,0,0}, {28,0,0}, {15,7,6}, {77,0,0}, {67,0,0}, {10,5,2}, 
{12,8,1}, {10,6,4}, {13,0,0}, {146,0,0}, {13,4,3}, {25,0,0}, 
{23,22,16}, {12,9,7}, {237,0,0}, {13,7,6}, {85,0,0}, {130,0,0}, 
{14,13,3}, {88,0,0}, {7,5,2}, {11,6,1}, {35,0,0}, {10,4,3}, 
{93,0,0}, {9,6,4}, {13,6,3}, {86,0,0}, {19,0,0}, {9,2,1}, 
{273,0,0}, {14,12,9}, {7,6,1}, {30,0,0}, {9,5,2}, {201,0,0}, 
{215,0,0}, {6,4,3}, {105,0,0}, {10,7,5}, {165,0,0}, {105,0,0}, 
{19,13,6}, {31,0,0}, {127,0,0}, {10,4,2}, {81,0,0}, {19,10,4}, 
{45,0,0}, {211,0,0}, {19,10,3}, {200,0,0}, {295,0,0}, {9,8,5}, 
{9,0,0}, {12,6,5}, {297,0,0}, {68,0,0}, {11,6,5}, {133,0,0}, 
{251,0,0}, {13,8,4}, {223,0,0}, {6,5,2}, {7,4,2}, {307,0,0}, 
{9,2,1}, {101,0,0}, {39,0,0}, {14,10,4}, {217,0,0}, {14,9,1}, 
{6,5,1}, {16,0,0}, {14,3,2}, {11,0,0}, {119,0,0}, {11,3,2}, 
{11,6,5}, {11,8,4}, {249,0,0}, {5,0,0}, {13,3,1}, {37,0,0}, 
{3,0,0}, {14,0,0}, {93,0,0}, {10,8,7}, {33,0,0}, {88,0,0}, 
{7,5,4}, {38,0,0}, {55,0,0}, {15,4,2}, {11,0,0}, {12,11,4}, 
{21,0,0}, {107,0,0}, {11,9,8}, {33,0,0}, {10,7,2}, {18,7,3}, 
{147,0,0}, {5,4,2}, {153,0,0}, {15,0,0}, {11,6,5}, {28,0,0}, 
{11,7,4}, {6,3,1}, {31,0,0}, {8,4,3}, {15,5,3}, {66,0,0}, 
{23,16,9}, {11,9,3}, {171,0,0}, {11,6,1}, {209,0,0}, {4,3,1}, 
{197,0,0}, {13,0,0}, {19,14,6}, {14,0,0}, {79,0,0}, {13,6,2}, 
{299,0,0}, {15,8,2}, {169,0,0}, {177,0,0}, {23,10,2}, {267,0,0}, 
{215,0,0}, {15,10,1}, {75,0,0}, {16,4,2}, {37,0,0}, {12,7,1}, 
{8,3,2}, {17,0,0}, {12,11,8}, {15,8,5}, {15,0,0}, {4,3,1}, 
{13,12,4}, {92,0,0}, {5,4,3}, {41,0,0}, {23,0,0}, {7,4,1}, 
{183,0,0}, {16,7,1}, {165,0,0}, {150,0,0}, {9,6,4}, {9,0,0}, 
{231,0,0}, {16,10,4}, {207,0,0}, {9,6,5}, {5,0,0}, {180,0,0}, 
{4,3,2}, {58,0,0}, {147,0,0}, {8,6,2}, {343,0,0}, {8,7,2}, 
{11,6,1}, {44,0,0}, {13,8,6}, {5,0,0}, {347,0,0}, {18,16,8}, 
{135,0,0}, {9,8,3}, {85,0,0}, {90,0,0}, {13,11,1}, {258,0,0}, 
{351,0,0}, {10,6,4}, {19,0,0}, {7,6,1}, {309,0,0}, {18,0,0}, 
{13,10,3}, {158,0,0}, {19,0,0}, {12,10,1}, {45,0,0}, {7,6,1}, 
{233,0,0}, {98,0,0}, {11,6,5}, {3,0,0}, {83,0,0}, {16,14,9}, 
{6,5,3}, {9,7,4}, {22,19,9}, {168,0,0}, {19,17,4}, {120,0,0}, 
{14,5,2}, {17,15,6}, {7,0,0}, {10,8,6}, {185,0,0}, {93,0,0}, 
{15,14,7}, {29,0,0}, {375,0,0}, {10,8,3}, {13,0,0}, {17,16,2}, 
{329,0,0}, {68,0,0}, {13,9,6}, {92,0,0}, {12,10,3}, {7,6,3}, 
{17,10,3}, {5,2,1}, {9,6,1}, {30,0,0}, {9,7,3}, {253,0,0}, 
{143,0,0}, {7,4,1}, {9,4,1}, {12,10,4}, {53,0,0}, {25,0,0}, 
{9,7,1}, {217,0,0}, {15,13,9}, {14,9,2}, {75,0,0}, {8,7,2}, 
{21,0,0}, {7,0,0}, {14,3,2}, {15,0,0}, {159,0,0}, {12,10,8}, 
{29,0,0}, {10,3,1}, {21,0,0}, {333,0,0}, {11,8,2}, {52,0,0}, 
{119,0,0}, {16,9,7}, {123,0,0}, {15,11,2}, {17,0,0}, {9,0,0}, 
{11,6,4}, {38,0,0}, {255,0,0}, {12,10,7}, {189,0,0}, {4,3,1}, 
{17,10,7}, {49,0,0}, {13,5,2}, {149,0,0}, {15,0,0}, {14,7,5}, 
{10,9,2}, {8,6,5}, {61,0,0}, {54,0,0}, {11,5,1}, {144,0,0}, 
{47,0,0}, {11,10,7}, {105,0,0}, {2,0,0}, {105,0,0}, {136,0,0}, 
{11,4,1}, {253,0,0}, {111,0,0}, {13,10,5}, {159,0,0}, {10,7,1}, 
{7,5,3}, {29,0,0}, {19,10,3}, {119,0,0}, {207,0,0}, {17,15,4}, 
{35,0,0}, {14,0,0}, {349,0,0}, {6,3,2}, {21,10,6}, {1,0,0}, 
{75,0,0}, {9,5,2}, {145,0,0}, {11,7,6}, {301,0,0}, {378,0,0}, 
{13,3,1}, {352,0,0}, {12,7,4}, {12,8,1}, {149,0,0}, {6,5,4}, 
{12,9,8}, {11,0,0}, {15,7,5}, {78,0,0}, {99,0,0}, {17,16,12}, 
{173,0,0}, {8,7,1}, {13,9,8}, {147,0,0}, {19,18,10}, {127,0,0}, 
{183,0,0}, {12,4,1}, {31,0,0}, {11,8,6}, {173,0,0}, {12,0,0}, 
{7,5,3}, {113,0,0}, {207,0,0}, {18,15,5}, {1,0,0}, {13,7,6}, 
{21,0,0}, {35,0,0}, {12,7,2}, {117,0,0}, {123,0,0}, {12,10,2}, 
{143,0,0}, {14,4,1}, {15,9,7}, {204,0,0}, {7,5,1}, {91,0,0}, 
{4,2,1}, {8,6,3}, {183,0,0}, {12,10,7}, {77,0,0}, {36,0,0}, 
{14,9,6}, {221,0,0}, {7,6,5}, {16,14,13}, {31,0,0}, {16,15,7}, 
{365,0,0}, {403,0,0}, {10,3,2}, {11,4,3}, {31,0,0}, {10,9,4}, 
{177,0,0}, {16,6,1}, {22,6,5}, {417,0,0}, {15,13,12}, {217,0,0}, 
{207,0,0}, {7,5,4}, {10,7,1}, {11,6,1}, {45,0,0}, {24,0,0}, 
{12,11,9}, {77,0,0}, {21,20,13}, {9,6,5}, {189,0,0}, {8,3,2}, 
{13,12,10}, {260,0,0}, {16,9,7}, {168,0,0}, {131,0,0}, {7,6,3}, 
{305,0,0}, {10,9,6}, {13,9,4}, {143,0,0}, {12,9,3}, {18,0,0}, 
{15,8,5}, {20,9,6}, {103,0,0}, {15,4,2}, {201,0,0}, {36,0,0}, 
{9,5,2}, {31,0,0}, {11,7,2}, {6,2,1}, {7,0,0}, {13,6,4}, 
{9,8,7}, {19,0,0}, {17,10,6}, {15,0,0}, {9,3,1}, {178,0,0}, 
{8,7,6}, {12,6,5}, {177,0,0}, {230,0,0}, {24,9,3}, {222,0,0}, 
{3,0,0}, {16,13,12}, {121,0,0}, {10,4,2}, {161,0,0}, {39,0,0}, 
{17,15,13}, {62,0,0}, {223,0,0}, {15,12,2}, {65,0,0}, {12,6,3}, 
{101,0,0}, {59,0,0}, {5,4,3}, {17,0,0}, {5,3,2}, {13,8,3}, 
{10,9,7}, {12,8,2}, {5,4,3}, {75,0,0}, {19,17,8}, {55,0,0}, 
{99,0,0}, {10,7,4}, {115,0,0}, {9,8,6}, {385,0,0}, {186,0,0}, 
{15,6,3}, {9,4,1}, {12,10,5}, {10,8,1}, {135,0,0}, {5,2,1}, 
{317,0,0}, {7,0,0}, {19,6,1}, {294,0,0}, {35,0,0}, {13,12,6}, 
{119,0,0}, {98,0,0}, {93,0,0}, {68,0,0}, {21,15,3}, {108,0,0}, 
{75,0,0}, {12,6,5}, {411,0,0}, {12,7,2}, {13,7,2}, {21,0,0}, 
{15,10,8}, {412,0,0}, {439,0,0}, {10,7,6}, {41,0,0}, {13,9,6}, 
{8,5,2}, {10,0,0}, {15,7,2}, {141,0,0}, {159,0,0}, {13,12,10}, 
{291,0,0}, {10,9,1}, {105,0,0}, {24,0,0}, {11,2,1}, {198,0,0}, 
{27,0,0}, {6,3,1}, {439,0,0}, {10,3,1}, {49,0,0}, {168,0,0}, 
{13,11,9}, {463,0,0}, {10,9,3}, {13,9,8}, {15,8,3}, {18,16,8}, 
{15,14,11}, {7,0,0}, {19,9,8}, {12,6,3}, {7,4,3}, {15,14,5}, 
{8,6,3}, {10,9,7}, {361,0,0}, {230,0,0}, {15,9,6}, {24,0,0}, 
{407,0,0}, {16,7,2}, {189,0,0}, {62,0,0}, {189,0,0}, {112,0,0}, 
{22,21,10}, {91,0,0}, {79,0,0}, {12,10,5}, {23,0,0}, {7,6,1}, 
{57,0,0}, {139,0,0}, {24,15,6}, {14,0,0}, {83,0,0}, {16,9,1}, 
{35,0,0}, {9,7,4}, {117,0,0}, {65,0,0}, {21,9,6}, {21,0,0}, 
{195,0,0}, {23,11,10}, {327,0,0}, {17,14,3}, {417,0,0}, {13,0,0}, 
{15,8,6}, {107,0,0}, {19,10,6}, {18,15,3}, {59,0,0}, {12,10,4}, 
{9,7,5}, {283,0,0}, {13,9,6}, {62,0,0}, {427,0,0}, {14,7,3}, 
{8,7,4}, {15,8,3}, {105,0,0}, {27,0,0}, {7,3,1}, {103,0,0}, 
{551,0,0}, {10,6,1}, {6,4,1}, {11,6,4}, {129,0,0}, {9,0,0}, 
{9,4,2}, {277,0,0}, {31,0,0}, {13,12,5}, {141,0,0}, {12,7,3}, 
{357,0,0}, {7,2,1}, {11,9,7}, {227,0,0}, {131,0,0}, {7,6,3}, 
{23,0,0}, {20,17,3}, {13,4,1}, {90,0,0}, {15,3,2}, {241,0,0}, 
{75,0,0}, {13,6,1}, {307,0,0}, {8,7,3}, {245,0,0}, {66,0,0}, 
{15,11,2}, {365,0,0}, {18,16,11}, {11,10,1}, {19,0,0}, {8,6,1}, 
{189,0,0}, {133,0,0}, {12,7,2}, {114,0,0}, {27,0,0}, {6,5,1}, 
{15,5,2}, {17,14,5}, {133,0,0}, {476,0,0}, {11,9,3}, {16,0,0}, 
{375,0,0}, {15,8,6}, {25,0,0}, {17,11,6}, {77,0,0}, {87,0,0}, 
{5,3,2}, {134,0,0}, {171,0,0}, {13,8,4}, {75,0,0}, {8,3,1}, 
{233,0,0}, {196,0,0}, {9,8,7}, {173,0,0}, {15,14,12}, {13,6,5}, 
{281,0,0}, {9,8,2}, {405,0,0}, {114,0,0}, {15,9,6}, {171,0,0}, 
{287,0,0}, {8,4,2}, {43,0,0}, {4,2,1}, {513,0,0}, {273,0,0}, 
{11,10,6}, {118,0,0}, {243,0,0}, {14,7,1}, {203,0,0}, {9,5,2}, 
{257,0,0}, {302,0,0}, {27,25,9}, {393,0,0}, {91,0,0}, {12,10,6}, 
{413,0,0}, {15,14,9}, {18,16,1}, {255,0,0}, {12,9,7}, {234,0,0}, 
{167,0,0}, {16,13,10}, {27,0,0}, {15,6,2}, {433,0,0}, {105,0,0}, 
{25,10,2}, {151,0,0}, {427,0,0}, {13,9,8}, {49,0,0}, {10,6,4}, 
{153,0,0}, {4,0,0}, {17,7,5}, {54,0,0}, {203,0,0}, {16,15,1}, 
{16,14,7}, {13,6,1}, {25,0,0}, {14,0,0}, {15,5,3}, {187,0,0}, 
{15,13,10}, {13,10,5}, {97,0,0}, {11,10,9}, {19,10,4}, {589,0,0}, 
{31,30,2}, {289,0,0}, {9,6,4}, {11,8,6}, {21,0,0}, {7,4,1}, 
{7,4,2}, {77,0,0}, {5,3,2}, {119,0,0}, {7,0,0}, {9,5,2}, 
{345,0,0}, {17,10,8}, {333,0,0}, {17,0,0}, {16,9,7}, {168,0,0}, 
{15,13,4}, {11,10,1}, {217,0,0}, {18,11,10}, {189,0,0}, {216,0,0}, 
{12,7,5}, {229,0,0}, {231,0,0}, {12,9,3}, {223,0,0}, {10,9,1}, 
{153,0,0}, {470,0,0}, {23,16,6}, {99,0,0}, {10,4,3}, {9,8,4}, 
{12,10,1}, {14,9,6}, {201,0,0}, {38,0,0}, {15,14,2}, {198,0,0}, 
{399,0,0}, {14,11,5}, {75,0,0}, {11,10,1}, {77,0,0}, {16,12,8}, 
{20,17,15}, {326,0,0}, {39,0,0}, {14,12,9}, {495,0,0}, {8,3,2}, 
{333,0,0}, {476,0,0}, {15,14,2}, {164,0,0}, {19,0,0}, {12,4,2}, 
{8,6,3}, {13,12,3}, {12,11,5}, {129,0,0}, {12,9,3}, {52,0,0}, 
{10,8,3}, {17,16,2}, {337,0,0}, {12,9,3}, {397,0,0}, {277,0,0}, 
{21,11,3}, {73,0,0}, {11,6,1}, {7,5,4}, {95,0,0}, {11,3,2}, 
{617,0,0}, {392,0,0}, {8,3,2}, {75,0,0}, {315,0,0}, {15,6,4}, 
{125,0,0}, {6,5,2}, {15,9,7}, {348,0,0}, {15,6,1}, {553,0,0}, 
{6,3,2}, {10,9,7}, {553,0,0}, {14,10,4}, {237,0,0}, {39,0,0}, 
{17,14,6}, {371,0,0}, {255,0,0}, {8,4,1}, {131,0,0}, {14,6,1}, 
{117,0,0}, {98,0,0}, {5,3,2}, {56,0,0}, {655,0,0}, {9,5,2}, 
{239,0,0}, {11,8,4}, {1,0,0}, {134,0,0}, {15,9,5}, {88,0,0}, 
{10,5,3}, {10,9,4}, {181,0,0}, {15,11,2}, {609,0,0}, {52,0,0}, 
{19,18,10}, {100,0,0}, {7,6,3}, {15,8,2}, {183,0,0}, {18,7,6}, 
{10,9,2}, {130,0,0}, {11,5,1}, {12,0,0}, {219,0,0}, {13,10,7}, 
{11,0,0}, {19,9,4}, {129,0,0}, {3,0,0}, {17,15,5}, {300,0,0}, 
{17,13,9}, {14,6,5}, {97,0,0}, {13,8,3}, {601,0,0}, {55,0,0}, 
{8,3,1}, {92,0,0}, {127,0,0}, {12,11,2}, {81,0,0}, {15,10,8}, 
{13,2,1}, {47,0,0}, {14,13,6}, {194,0,0}, {383,0,0}, {25,14,11}, 
{125,0,0}, {20,19,16}, {429,0,0}, {282,0,0}, {10,9,6}, {342,0,0}, 
{5,3,2}, {15,9,4}, {33,0,0}, {9,4,2}, {49,0,0}, {15,0,0}, 
{11,6,2}, {28,0,0}, {103,0,0}, {18,17,8}, {27,0,0}, {11,6,5}, 
{33,0,0}, {17,0,0}, {11,10,6}, {387,0,0}, {363,0,0}, {15,10,9}, 
{83,0,0}, {7,6,4}, {357,0,0}, {13,12,4}, {14,13,7}, {322,0,0}, 
{395,0,0}, {16,5,1}, {595,0,0}, {13,10,3}, {421,0,0}, {195,0,0}, 
{11,3,2}, {13,0,0}, {16,12,3}, {14,3,1}, {315,0,0}, {26,10,5}, 
{297,0,0}, {52,0,0}, {9,4,2}, {314,0,0}, {243,0,0}, {16,14,9}, 
{185,0,0}, {12,5,3}, {13,5,2}, {575,0,0}, {12,9,3}, {39,0,0}, 
{311,0,0}, {13,5,2}, {181,0,0}, {20,18,14}, {49,0,0}, {25,0,0}, 
{11,4,1}, {77,0,0}, {17,11,10}, {15,14,8}, {21,0,0}, {17,10,5}, 
{69,0,0}, {49,0,0}, {11,10,2}, {32,0,0}, {411,0,0}, {21,16,3}, 
{11,7,4}, {22,10,3}, {85,0,0}, {140,0,0}, {9,8,6}, {252,0,0}, 
{279,0,0}, {9,5,2}, {307,0,0}, {17,10,4}, {13,12,9}, {94,0,0}, 
{13,11,4}, {49,0,0}, {17,11,10}, {16,12,5}, {25,0,0}, {6,5,2}, 
{12,5,1}, {80,0,0}, {8,3,2}, {246,0,0}, {11,5,2}, {11,10,2}, 
{599,0,0}, {18,12,10}, {189,0,0}, {278,0,0}, {10,9,3}, {399,0,0}, 
{299,0,0}, {13,10,6}, {277,0,0}, {13,10,6}, {69,0,0}, {220,0,0}, 
{13,10,3}, {229,0,0}, {18,11,10}, {16,15,1}, {27,0,0}, {18,9,3}, 
{473,0,0}, {373,0,0}, {18,17,7}, {60,0,0}, {207,0,0}, {13,9,8}, 
{22,20,13}, {25,18,7}, {225,0,0}, {404,0,0}, {21,6,2}, {46,0,0}, 
{6,2,1}, {17,12,6}, {75,0,0}, {4,2,1}, {365,0,0}, {445,0,0}, 
{11,7,1}, {44,0,0}, {10,8,5}, {12,5,2}, {63,0,0}, {17,4,2}, 
{189,0,0}, {557,0,0}, {19,12,2}, {252,0,0}, {99,0,0}, {10,8,5}, 
{65,0,0}, {14,9,3}, {9,0,0}, {119,0,0}, {8,5,2}, {339,0,0}, 
{95,0,0}, {12,9,7}, {7,0,0}, {13,10,2}, {77,0,0}, {127,0,0}, 
{21,10,7}, {319,0,0}, {667,0,0}, {17,10,3}, {501,0,0}, {18,12,9}, 
{9,8,5}, {17,0,0}, {20,9,2}, {341,0,0}, {731,0,0}, {7,6,5}, 
{647,0,0}, {10,4,2}, {121,0,0}, {20,0,0}, {21,19,13}, {574,0,0}, 
{399,0,0}, {15,10,7}, {85,0,0}, {16,8,3}, {169,0,0}, {15,0,0}, 
{12,7,5}, {568,0,0}, {10,7,1}, {18,2,1}, {3,0,0}, {14,3,2}, 
{13,7,3}, {643,0,0}, {14,11,1}, {548,0,0}, {783,0,0}, {14,11,1}, 
{317,0,0}, {7,6,4}, {153,0,0}, {87,0,0}, {15,13,1}, {231,0,0}, 
{11,5,3}, {18,13,7}, {771,0,0}, {30,20,11}, {15,6,3}, {103,0,0}, 
{13,4,3}, {182,0,0}, {211,0,0}, {17,6,1}, {27,0,0}, {13,12,10}, 
{15,14,10}, {17,0,0}, {13,11,5}, {69,0,0}, {11,5,1}, {18,6,1}, 
{603,0,0}, {10,4,2}, {741,0,0}, {668,0,0}, {17,15,3}, {147,0,0}, 
{227,0,0}, {15,10,9}, {37,0,0}, {16,6,1}, {173,0,0}, {427,0,0}, 
{7,5,1}, {287,0,0}, {231,0,0}, {20,15,10}, {18,9,1}, {14,12,5}, 
{16,5,1}, {310,0,0}, {18,13,1}, {434,0,0}, {579,0,0}, {18,13,8}, 
{45,0,0}, {12,8,3}, {16,9,5}, {53,0,0}, {19,15,10}, {16,0,0}, 
{17,6,5}, {17,10,1}, {37,0,0}, {17,10,9}, {21,13,7}, {99,0,0}, 
{17,9,6}, {176,0,0}, {271,0,0}, {18,17,13}, {459,0,0}, {21,17,10}, 
{6,5,2}, {202,0,0}, {5,4,3}, {90,0,0}, {755,0,0}, {15,7,2}, 
{363,0,0}, {8,4,2}, {129,0,0}, {20,0,0}, {11,6,2}, {135,0,0}, 
{15,8,7}, {14,13,2}, {10,4,3}, {24,13,10}, {19,14,11}, {31,0,0}, 
{15,8,6}, {758,0,0}, {16,11,5}, {16,5,1}, {359,0,0}, {23,18,17}, 
{501,0,0}, {29,0,0}, {15,6,3}, {201,0,0}, {459,0,0}, {12,10,7}, 
{225,0,0}, {22,17,13}, {24,22,5}, {161,0,0}, {14,11,3}, {52,0,0}, 
{19,17,6}, {21,14,12}, {93,0,0}, {13,10,3}, {201,0,0}, {178,0,0}, 
{15,12,5}, {250,0,0}, {7,6,4}, {17,13,6}, {221,0,0}, {13,11,8}, 
{17,14,9}, {113,0,0}, {17,14,10}, {300,0,0}, {39,0,0}, {18,13,3}, 
{261,0,0}, {15,14,8}, {753,0,0}, {8,4,3}, {11,10,5}, {94,0,0}, 
{15,13,1}, {10,4,2}, {14,11,10}, {8,6,2}, {461,0,0}, {418,0,0}, 
{19,14,6}, {403,0,0}, {267,0,0}, {10,9,2}, {259,0,0}, {20,4,3}, 
{869,0,0}, {173,0,0}, {19,18,2}, {369,0,0}, {255,0,0}, {22,12,9}, 
{567,0,0}, {20,11,7}, {457,0,0}, {482,0,0}, {6,3,2}, {775,0,0}, 
{19,17,6}, {6,4,3}, {99,0,0}, {15,14,8}, {6,5,2}, {165,0,0}, 
{8,3,2}, {13,12,10}, {25,21,17}, {17,14,9}, {105,0,0}, {17,15,14}, 
{10,3,2}, {250,0,0}, {25,6,5}, {327,0,0}, {279,0,0}, {13,6,5}, 
{371,0,0}, {15,9,4}, {117,0,0}, {486,0,0}, {10,9,3}, {217,0,0}, 
{635,0,0}, {30,27,17}, {457,0,0}, {16,6,2}, {57,0,0}, {439,0,0}, 
{23,21,6}, {214,0,0}, {20,13,6}, {20,16,1}, {819,0,0}, {15,11,8}, 
{593,0,0}, {190,0,0}, {17,14,3}, {114,0,0}, {21,18,3}, {10,5,2}, 
{12,9,5}, {8,6,3}, {69,0,0}, {312,0,0}, {22,5,2}, {502,0,0}, 
{843,0,0}, {15,10,3}, {747,0,0}, {6,5,2}, {101,0,0}, {123,0,0}, 
{19,16,9}, {521,0,0}, {171,0,0}, {16,7,2}, {12,6,5}, {22,21,20}, 
{545,0,0}, {163,0,0}, {23,18,1}, {479,0,0}, {495,0,0}, {13,6,5}, 
{11,0,0}, {17,5,2}, {18,8,1}, {684,0,0}, {7,5,1}, {9,0,0}, 
{18,11,3}, {22,20,13}, {273,0,0}, {4,3,2}, {381,0,0}, {51,0,0}, 
{18,13,7}, {518,0,0}, {9,5,1}, {14,12,3}, {243,0,0}, {21,17,2}, 
{53,0,0}, {836,0,0}, {21,10,2}, {66,0,0}, {12,10,7}, {13,9,8}, 
{339,0,0}, {16,11,5}, {901,0,0}, {180,0,0}, {16,13,3}, {49,0,0}, 
{6,3,2}, {15,4,1}, {16,13,6}, {18,15,12}, {885,0,0}, {39,0,0}, 
{11,9,4}, {688,0,0}, {16,15,7}, {13,10,6}, {13,0,0}, {25,23,12}, 
{149,0,0}, {260,0,0}, {11,9,1}, {53,0,0}, {11,0,0}, {12,4,2}, 
{9,7,5}, {11,8,1}, {121,0,0}, {261,0,0}, {10,5,2}, {199,0,0}, 
{20,4,3}, {17,9,2}, {13,9,4}, {12,8,7}, {253,0,0}, {174,0,0}, 
{15,4,2}, {370,0,0}, {9,6,1}, {16,10,9}, {669,0,0}, {20,10,9}, 
{833,0,0}, {353,0,0}, {17,13,2}, {29,0,0}, {371,0,0}, {9,8,5}, 
{8,7,1}, {19,8,7}, {12,11,10}, {873,0,0}, {26,11,2}, {12,9,1}, 
{10,7,2}, {13,6,1}, {235,0,0}, {26,24,19}, {733,0,0}, {778,0,0}, 
{12,11,1}, {344,0,0}, {931,0,0}, {16,6,4}, {945,0,0}, {21,19,14}, 
{18,13,11}, {67,0,0}, {20,15,10}, {462,0,0}, {14,5,1}, {10,9,6}, 
{18,11,10}, {16,9,7}, {477,0,0}, {105,0,0}, {11,3,2}, {468,0,0}, 
{23,16,15}, {16,15,6}, {327,0,0}, {23,10,4}, {357,0,0}, {25,0,0}, 
{17,16,7}, {31,0,0}, {7,5,2}, {16,7,6}, {277,0,0}, {14,13,6}, 
{413,0,0}, {103,0,0}, {15,10,1}, {231,0,0}, {747,0,0}, {5,2,1}, 
{113,0,0}, {20,10,7}, {15,9,6}, {11,0,0}, {27,22,18}, {91,0,0}, 
{51,0,0}, {18,13,12}, {603,0,0}, {10,7,3}, {9,0,0}, {121,0,0}, 
{15,14,6}, {17,0,0}, {16,11,2}, {23,15,6}, {279,0,0}, {16,12,6}, 
{89,0,0}, {371,0,0}, {17,15,2}, {771,0,0}, {99,0,0}, {7,6,3}, 
{21,0,0}, {10,7,5}, {801,0,0}, {26,0,0}, {25,19,14}, {175,0,0}, 
{10,7,2}, {20,5,4}, {12,11,1}, {22,5,1}, {165,0,0}, {841,0,0}, 
{25,19,17}, {238,0,0}, {11,8,6}, {22,21,4}, {33,0,0}, {8,7,6}, 
{14,9,2}, {113,0,0}, {13,11,5}, {311,0,0}, {891,0,0}, {20,16,14}, 
{555,0,0}, {23,14,8}, {133,0,0}, {546,0,0}, {6,3,2}, {103,0,0}, 
{15,0,0}, {10,7,3}, {307,0,0}, {14,10,1}, {15,12,2}, {367,0,0}, 
{13,10,6}, {169,0,0}, {22,21,11}, {12,10,8}, {441,0,0}, {17,12,7}, 
{917,0,0}, {205,0,0}, {26,23,13}, {54,0,0}, {459,0,0}, {17,15,4}, 
{19,15,4}, {5,4,2}, {9,7,6}, {42,0,0}, {21,15,7}, {330,0,0}, 
{20,7,3}, {20,7,2}, {81,0,0}, {19,14,1}, {349,0,0}, {165,0,0}, 
{40,35,9}, {274,0,0}, {475,0,0}, {11,10,3}, {93,0,0}, {12,7,4}, 
{13,12,2}, {386,0,0}, {7,6,2}, {881,0,0}, {143,0,0}, {9,8,4}, 
{71,0,0}, {19,18,3}, {16,11,6}, {155,0,0}, {7,2,1}, {735,0,0}, 
{16,8,7}, {9,7,4}, {45,0,0}, {7,6,4}, {12,11,3}, {3,0,0}, 
{19,14,13}
};




static
long FindTrinom(long n)
{
   if (n < 2) LogicError("tri--bad n");

   long k;

   for (k = 1; k <= n/2; k++)
      if (IterIrredTest(1 + GF2X(k,1) + GF2X(n,1)))
         return k;

   return 0;
}


static
long FindPent(long n, long& kk2, long& kk1)
{
   if (n < 4) LogicError("pent--bad n");

   long k1, k2, k3;

   for (k3 = 3; k3 < n; k3++)
      for (k2 = 2; k2 < k3; k2++) 
         for (k1 = 1; k1 < k2; k1++)
            if (IterIrredTest(1+GF2X(k1,1)+GF2X(k2,1)+GF2X(k3,1)+GF2X(n,1))) {
               kk2 = k2;
               kk1 = k1;
               return k3;
            }

   return 0;
}

void BuildSparseIrred(GF2X& f, long n)
{
   if (n <= 0) LogicError("SparseIrred: n <= 0");

   if (NTL_OVERFLOW(n, 1, 0)) 
      ResourceError("overflow in BuildSparseIrred");

   if (n == 1) {
      SetX(f);
      return;
   }

   if (n <= 2048) {
      if (GF2X_irred_tab[n][1] == 0) {
         clear(f);
         SetCoeff(f, n);
         SetCoeff(f, GF2X_irred_tab[n][0]);
         SetCoeff(f, 0);
      }
      else {
         clear(f);
         SetCoeff(f, n);
         SetCoeff(f, GF2X_irred_tab[n][0]);
         SetCoeff(f, GF2X_irred_tab[n][1]);
         SetCoeff(f, GF2X_irred_tab[n][2]);
         SetCoeff(f, 0);
      }

      return;
   }

   long k3, k2, k1;

   k3 = FindTrinom(n);
   if (k3) {
      clear(f);
      SetCoeff(f, n);
      SetCoeff(f, k3);
      SetCoeff(f, 0);
      return;
   }

   k3 = FindPent(n, k2, k1);
   if (k3) {
      clear(f);
      SetCoeff(f, n);
      SetCoeff(f, k3);
      SetCoeff(f, k2);
      SetCoeff(f, k1);
      SetCoeff(f, 0);
      return;
   }

   // the following is probably of only theoretical value...
   // it is reasonable to conjecture that for all n >= 2,
   // there is either an irreducible trinomial or pentanomial
   // of degree n.

   BuildIrred(f, n);
}

NTL_END_IMPL

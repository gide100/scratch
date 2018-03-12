#include <iostream>

struct Currency
{
  int Dollar;
  int Cents;
  Currency() : Dollar(0), Cents(0) {}

  /*REMOVED : operator double();*/
  operator double()  {
    return Dollar + (double)Cents/100;
  }
 
  double operator*(int nMultiplier)
  {
    return (Dollar+(double)Cents/100) * nMultiplier;
  }
  
  Currency& operator+=(const Currency& rhs){
      Dollar += rhs.Dollar;
      Cents += rhs.Cents;
      Dollar += Cents/ 100;
      Cents %= 100;
      return *this;
  }
};

template<typename T>
double GetAverage(T tArray[], int nElements)
{
   T tSum = T(); // tSum = 0
   for (int nIndex = 0; nIndex < nElements; ++nIndex) {
     tSum += tArray[nIndex];
   }
   
   // Whatever type of T is, convert to double
   return double(tSum) / nElements;
}


int main() {
  Currency currency[10];
  currency[0].Dollar = 20;
  auto res = GetAverage<Currency>(currency, 10);
  std::cout << res << std::endl;
}

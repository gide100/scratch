#include <iostream>

struct Currency
{
  int Dollar;
  int Cents;
  Currency() : Dollar(0), Cents(0) {}
  Currency( int dollar, int cents=0) : Dollar(dollar), Cents(cents) {}

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


template<typename T>
class Item {
    public:
        Item();
        void SetData(T data);
        void PrintData();
        Item<T>& operator+=( const Item<T>& other ); 
        Item<T>& operator/=( const Item<T>& other ); 

        template <typename U>
        void SetAverageFrom(U values[10], int size);
    private:
        T Data;
};

template<typename T>
Item<T>::Item() : Data( T() ) {
}
 
template<typename T>
void Item<T>::SetData(T data) {
    Data = data;
}
template<typename T>
void Item<T>::PrintData() {
    std::cout << Data;
}

template<typename T>
Item<T>& Item<T>::operator+=( const Item<T>& other ) { 
    Data += other.Data;
    return *this;
}

template<typename T>
Item<T>& Item<T>::operator/=( const Item<T>& other ) { 
    Data /= other.Data;
    return *this;
}

template<typename T>
    template<typename U>
    void Item<T>::SetAverageFrom(U values[10], int size) {
    U item;
    for (int i =0; i < size;++i) {
        item += values[i];
    }
    Data /= (double)size;
}

int main() {
  //Currency currency[10];
  //currency[0].Dollar = 20;
  //auto res = GetAverage<Currency>(currency, 10);
  //std::cout << res << std::endl;
    const int Size = 10;
    //Item<long> Values[Size]; 
    Item<Currency> Values[Size]; 
 
    for(int nItem = 0; nItem < Size; ++nItem) {
        Values[nItem].SetData(nItem*40);
    }
          
    Item<float> FloatItem;
    FloatItem.SetAverageFrom(Values, Size);
}


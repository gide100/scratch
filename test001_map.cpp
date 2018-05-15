#include <assert.h>
#include <map>
#include <limits>
#include <iostream>
#include <sstream>
#include <cassert>
#include <algorithm>

template<class K, class V>

class interval_map {
    friend void IntervalMapTest();
    template<typename KEY, typename VALUE> 
    friend bool operator==(const interval_map<KEY,VALUE>& lhs, const std::map<KEY,VALUE>& rhs);
private:
    std::map<K,V> m_map;
public:
    // constructor associates whole range of K with val by inserting (K_min, val)
    // into the map
    interval_map( V const& val) {
        std::cout << "Ctor >=" << std::numeric_limits<K>::lowest() << " " << val << std::endl;
        m_map.insert(m_map.begin(),std::make_pair(std::numeric_limits<K>::lowest(),val));
    }


    // Assign value val to interval [keyBegin, keyEnd). 
    // Overwrite previous values in this interval. 
    // Do not change values outside this interval.
    // Conforming to the C++ Standard Library conventions, the interval 
    // includes keyBegin, but excludes keyEnd.
    // If !( keyBegin < keyEnd ), this designates an empty interval, 
    // and assign must do nothing.
    void assign( K const& keyBegin, K const& keyEnd, V const& val ) {
        std::cout << "Assign >=" << keyBegin << " <" << keyEnd << " " << val << std::endl;
        if (keyBegin >= keyEnd) {
            return;
        }
        assert(!m_map.empty() && "map empty");
        // 1) Find less than or equal to keyBegin
        // 1a) If found less than
        // 1aa) if val is same as lower do nothing (store range)
        // 1ab) Else insert entry (store range)
        // 1b) If found equal 
        // 1ba) 
        
        // Find less than keyBegin
        auto itBegin = m_map.lower_bound(keyBegin); //GTE
        bool haveEqual = itBegin != m_map.end() && itBegin->first == keyBegin;
        // Find previous
        bool havePrev = itBegin!=m_map.begin();
        auto itPrev = itBegin;
        V valPrev;
        if (havePrev) {
            --itPrev; valPrev = itPrev->second;
        }

        
        if (haveEqual && havePrev) {
            // Previous and Equal, compare... same remove, different replace
            if (val == valPrev) {
                m_map.erase(itBegin);
                itBegin = itPrev;
                //std::cout << "HERE" << __LINE__ << std::endl;
            } else {
                itBegin->second = val;
            }
        } else if (havePrev) {
            // if same do nothing, otherwise (different) insert
            if (val != valPrev) {
                itBegin = m_map.insert(itPrev,std::make_pair(keyBegin,val));
            }
        } else if (haveEqual) {
            // Replace
            valPrev = itBegin->second; // havePrev = true;
            itBegin->second = val;
        } else {
            assert(false && "prev and equal not found (empty map)");
        }
        assert(itBegin->first <= keyBegin && "Must be less than or equal keyBegin");
        assert(itBegin->second == val && "Added first val");

        // Find less than keyEnd
        // (Reverse) find the first item less than keyEnd, exclude inserted keyBegin
        auto myrend = make_reverse_iterator(itBegin); 
        if (myrend != m_map.rend()) {
            std::cout << "HERE" << __LINE__ << std::endl;
            myrend = std::prev(myrend);
        }
        auto rit = lower_bound(m_map.rbegin(), myrend, keyEnd, 
                                [](const std::pair<const K,V>& l, K r) -> bool
                                { 
                                  return !(l.first < r);
                                } );

        typename std::map<K,V>::iterator it2; 
        std::cout << "rit=" << rit->first << " rbegin=" << m_map.rbegin()->first << " myrend=" << myrend->first << " itBegin=" << itBegin->first << std::endl;
        if (rit != myrend) {
            it2 = std::next(rit).base(); // Fwd iterator of found value
            if (it2 != m_map.begin()) { // Not first item, (i.e. not 0) set previous
                valPrev = rit->second;
            }
            auto itNext = std::next(it2); // Check for equal to, if equal inc iterator
            if ( (itNext != m_map.end()) && (itNext->first == keyEnd) ) {
                std::cout << "HERE " << __LINE__ << std::endl;
                it2 = itNext;
            }
            std::cout << "HERE " << __LINE__ << std::endl;
        } else {
            std::cout << "HERE " << __LINE__ << std::endl;
            // Nothing found set to keyBegin iterator
            it2 = itBegin;
        }
        std::cout << "itBegin=" << itBegin->first << " it2=" << it2->first << " valPrev=" << valPrev << std::endl;
        assert(it2 != m_map.end() && "it2 must have value");
        if (it2->first == keyEnd) {
            if (it2->second == val) {
                ++it2;
            }
        } else {
            std::cout << "HERE " << __LINE__ << std::endl;
            if (val != valPrev) {
                it2 = m_map.insert(++it2,std::make_pair(keyEnd,valPrev));
            } else {
                ++it2;
            }
        }
        m_map.erase(std::next(itBegin), it2);
    }

    // look-up of the value associated with key
    V const& operator[]( K const& key ) const {
        return ( --m_map.upper_bound(key) )->second;
    }

    std::string to_string() const {
        std::ostringstream os;
        for (const auto& kv: m_map) {
            os << kv.first << " -> " << kv.second << std::endl;
        }
        return os.str();
    }
};

template<class K, class V>
bool operator==(const interval_map<K,V>& lhs, const std::map<K,V>& rhs) {
    return lhs.m_map == rhs;
}
template<class K, class V>
bool operator!=(const interval_map<K,V>& lhs, const std::map<K,V>& rhs) {
    return !(lhs == rhs);
}

// Many solutions we receive are incorrect. Consider using a randomized test
// to discover the cases that your implementation does not handle correctly.
// We recommend to implement a function IntervalMapTest() here that tests the
// functionality of the interval_map, for example using a map of unsigned int
// intervals to char.
void IntervalMapTest() ;

int main() {
    IntervalMapTest();
}

void IntervalMapTest() {
    using Map = std::map<unsigned int, char>;
    interval_map<unsigned int, char> im0('A');
    im0.assign(3,5,'B');
    std::cout << "im0:\n" << im0.to_string() << std::endl;
    Map res00 { {0,'A'}, {3,'B'}, {5,'A'} };
    assert(im0 == res00);
    im0.assign(0,8,'A');
    std::cout << "im0:\n" << im0.to_string() << std::endl;
    Map res00a { {0,'A'} };
    assert(im0 == res00a);



    interval_map<unsigned int, char> im1('A');
    im1.assign(3,5,'B');
    std::cout << "im1:\n" << im1.to_string() << std::endl;
    Map res01 { {0,'A'}, {3,'B'}, {5,'A'} };
    assert(im1 == res01);

    im1.assign(4,8,'C');
    std::cout << "im1:\n" << im1.to_string() << std::endl;
    Map res02 { {0,'A'}, {3,'B'}, {4,'C'}, {8,'A'} };
    assert(im1 == res02);

    interval_map<unsigned int, char> im2('A');
    im2.assign(3,5,'B');
    im2.assign(4,8,'C');
    std::cout << "im2:\n" << im2.to_string() << std::endl;
    im2.assign(3,5,'A');
    std::cout << "im2:\n" << im2.to_string() << std::endl;
    Map res03 { {0,'A'}, {5,'C'}, {8,'A'} };
    assert(im2 == res03);

    interval_map<unsigned int, char> im3('A');
    im3.assign(3,5,'B');
    im3.assign(4,8,'C');
    im3.assign(3,8,'A');
    std::cout << "im3:\n" << im3.to_string() << std::endl;
    Map res04 { {0,'A'} };
    assert(im3 == res04);

    im3.assign(1,0,'Z');
    std::cout << "im3:\n" << im3.to_string() << std::endl;
    assert(im3 == res04);

    interval_map<unsigned int, char> im4('A');
    im4.assign(0,9,'Z');
    std::cout << "im4:\n" << im4.to_string() << std::endl;
    Map res05 { {0,'Z'}, {9,'A'} };
    assert(im4 == res05);
    im4.assign(2,5,'B');
    std::cout << "im4:\n" << im4.to_string() << std::endl;
    Map res06 { {0,'Z'}, {2,'B'}, {5,'Z'}, {9,'A'} };
    assert(im4 == res06);
    im4.assign(0,15,'A');
    std::cout << "im4:\n" << im4.to_string() << std::endl;
    Map res07 { {0,'A'} };
    assert(im4 == res07);

    interval_map<unsigned int, char> im5('A');
    im5.assign(0,1,'Z');
    im5.assign(3,5,'B');
    im5.assign(4,8,'C');
    im5.assign(0,8,'A');
    std::cout << "im5:\n" << im5.to_string() << std::endl;


}

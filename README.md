# scratch
Scratch area for testing code

## Synopsis

This is a testing area for some code regarding trading. It is based on the Boost framework and should run on Linux and Solaris.
At the top of the file there should be a short introduction and/ or overview that explains **what** the project is. This description should match descriptions added for package managers (Gemspec, package.json, etc.)

## Code Example

The main code is a MatchingEngine and code to recieve and send messages to the matching engine.

```cpp
std::cout << " *** SECDB ***" << std::endl;                                                               
an::SecurityDatabase secdb(an::ME);                                                                       
secdb.loadData("security_database.csv");                                                                  
                                                                                                              
std::cout << " *** TICK ***" << std::endl;                                                                
an::TickLadder tickdb;                                                                                    
tickdb.loadData("NXT_ticksize.txt");                                                                      

std::cout << " *** ORDERS ***" << std::endl;
auto lim01a = std::make_unique<an::LimitOrder >(  3,"Client2", an::ME,"APPL",an::SELL,10,12.23);

std::cout << " *** ME ***" << std::endl;                                                                  
an::MatchingEngine me(an::ME, secdb);    
me.applyOrder(std::move(lim01a));
```


There are two pair to the code the matching engine and messages and the transport and threading framework.

## Motivation

Just to understand how to write an exchange and how matching engines work.

## Installation

Requires b2 (Boost Build aka Jam), a C++11 complaint compiler and the Boost libraries 1.66 or higher.

## Tests

```shell
b2 && bin/gcc-gnu-7/debug/unittest_order --log_level=test_suit
```

## Contributors

Just me at the moment, I am contactable on e-mail, andrew at sprysoltuions(DOT)co(DOT)uk

## License

Probably BSD license not sure what is the best.


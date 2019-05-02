#ifndef _STOCK_H
#define _STOCK_H

#include <stdlib.h>

class Stock
{
public:
   Stock(int id, int base, int max): _id(id), _base(base), _value(base), _max(max) {}
   int get_id() {return _id;}
   int get_value() {return _value;}
   int modify_value(int amt);
   void reset() {_value = _base;}
private:
   int _id;				// stock's unique id
   int _base;				// base value of stock
   int _max;				// max value before stock split
   int _value;				// current value
};

class StockList
{
public:
   StockList(int stocks=6, int base=100, int max=200);
   ~StockList();
   Stock* get_stock(int id);
   void reset_stocks() {for(int i=0;i<6;i++) _list[i]->reset();}
private:
   // yes it's hardcoded.. there's not enough time to make it elegant.
   Stock* _list[6];			// list of stocks
   int    _len;				// length of list
};

#endif

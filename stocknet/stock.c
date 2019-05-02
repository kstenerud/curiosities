#include "stock.h"
#include "player.h"

extern PlayerList Plist;

//------------------------------------------------------------------------------
// name:   Stock::modify_value
// desc:   modify the value of this stock
// inputs: amt - amount to modify by (positive or negative)
// output: TRUE/FALSE (success/failure)
int Stock::modify_value(int amt)
{
   if(this == NULL)
      return 1==0;

   _value += amt;
   if(_value <= 0)		// check for crash
   {
      Plist.stock_crash(_id);
      _value = _base;
      return 1==0;
   }

   if(_value > _max)		// check for split
   {
      Plist.stock_split(_id);
      _value = _base;
      return 1==0;
   }

   return 1==1;
}

//==============================================================================

StockList::StockList(int stocks, int base, int max)
: _len(stocks)
{
   for(int i=0;i<_len;i++)
      _list[i] = new Stock(i, base, max);
}

StockList::~StockList()
{
   for(int i=0;i<_len;i++)
      delete _list[i];
}

//------------------------------------------------------------------------------
// name:   Stock::get_stock
// desc:   find a stock by its id
// inputs: id - the stock to search for
// output: Stock* - the stock found or NULL if error
Stock* StockList::get_stock(int id)
{
   if(id < 0 || id >= _len)
      return NULL;

   return _list[id];
}

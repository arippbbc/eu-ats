class AlgoBaseClass 
{
public:
	virtual void tickPrice( TickerId tickerId, TickType tickType, int price ) = 0;
	virtual void tickSize( TickerId tickerId, TickType tickType, int size ) = 0;
};

// one particular algorithm implementation
class MyCoolAlgoCrossingMAs : public AlgoBaseClass
{
	void tickPrice( TickerId tickerId, TickType tickType, int price ){ /* decide buy or sell */}
	void tickSize( TickerId tickerId, TickType tickType, int size ) { /* decide buy or sell */}
};

// another algorithm implementation
class MyEvenCoolerAlgoRandomBuyNeverSell : public AlgoBaseClass
{
	void tickPrice( TickerId tickerId, TickType tickType, int price ){ /* decide buy or sell */}
	void tickSize( TickerId tickerId, TickType tickType, int size ) { /* decide buy or sell */}
};

// class stock_class
class Forex
{
private:
	std::vector<AlgoBaseClass*> subscribed_algos;

	void tickPrice( TickerId tickerId, TickType tickType, int price )
	{
		for(std::vector<AlgoBaseClass*>::iterator ialgo = subscribed_algos.begin(); ialgo != subscribed_algos.end(); ialgo++)
		{
			(*ialgo)->tickPrice(tickerId, tickType, price );	// forward tickPrice() event to subscribed algos (if any)
		}
	}

	void tickSize( TickerId tickerId, TickType tickType, int size )
	{
		for(std::vector<AlgoBaseClass*>::iterator ialgo = subscribed_algos.begin(); ialgo != subscribed_algos.end(); ialgo++)
		{
			(*ialgo)->tickPrice(tickerId, tickType, size );	// forward tickSize() event to subscribed algos (if any)
		}
	}
public:
	// pass pointer to your algo instance to subscribe your algo for this stock ticker
	void subscribeAlgo(AlgoBaseClass * p_algo)
	{
		auto it = std::find(subscribed_algos.begin(), subscribed_algos.end(), p_algo);
		if(it != subscribed_algos.end())
			subscribed_algos.push_back(p_algo);
		else
			std::cerr << "WARNING: ignoring an attempt to subscribe already subscribed algo\n";
	}

	void unsubscribeAlgo(AlgoBaseClass * p_algo)
	{
		auto it = std::find(subscribed_algos.begin(), subscribed_algos.end(), p_algo);
		if(it != subscribed_algos.end())
			subscribed_algos.erase(it);
	}
};


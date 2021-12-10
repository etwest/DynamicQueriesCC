
// maintains the tiers of the algorithm
// and the spanning forest of the entire graph
class GraphTiers {
	private:
		void refresh();

	public:
		GraphTiers();
		~GraphTiers();

		// apply an edge update
		void update();

		// query for the connected components of the graph
		// open question: how to use ETTs to solve CC?
		// open question: is CC the right query? query: A connected to B better?
		void get_cc();
};

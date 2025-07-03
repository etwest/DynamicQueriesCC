#include "DynamicCC.h"
#include <binary_file_stream.h>

#include <sys/resource.h> // for rusage

// #define PrintQuery

static double get_max_mem_used() {
  struct rusage data;
  getrusage(RUSAGE_SELF, &data);
  return (double) data.ru_maxrss / 1024.0;
}

int get_dep(DynamicCC &g, int u, vector<int> &dep) {
	if(u == -1) return -1;
	if(dep[u] != -1) return dep[u];
	return (dep[u] = get_dep(g, g.nodes[u].p, dep) + 1);
}

void print_dep(DynamicCC &g) {
	vector<int> dep(g.n,-1);
	int max_dep = 0;
	long long sum_dep = 0;
	for(int u = 0; u < g.n; ++u) {
		sum_dep += get_dep(g, u, dep);
		max_dep = max(max_dep, dep[u]);
	}
	printf( "max_dep = %d, avg_dep = %0.3lf\n", max_dep, 1.0*sum_dep/g.n);
}

void print_avg(DynamicCC &g) {
	
	printf("replacesuccessnum = %d, removenum = %d\n", g.replacesuccessnum, g.removenum);
	g.avgi = g.avgi / g.replacesuccessnum;
	g.avgq = g.avgq / g.removenum;
	g.avgdelta = g.avgdelta / g.removenum;
	g.avg = g.avg / g.removenum;
	printf("avgi = %0.3lf, avgq = %0.3lf, avgdelta = %0.3lf, avg = %0.3lf\n", g.avgi, g.avgq, g.avgdelta, g.avg);
	printf("maxi = %d, maxq = %d, maxdelta = %d, maxavg = %0.3lf\n", g.maxi, g.maxq, g.maxdelta, g.maxavg);
	printf("needrerootnum = %d\n", g.needrerootnum);
}

void sample(string path, long long n_edges, bool use_union_find = true) {
	DynamicCC g(path, true, use_union_find);
	clock_t t = clock();
	g.init();
	printf( "Initialize time = %0.3lf sec\n", (clock()-t)*1.0/CLOCKS_PER_SEC);
	print_dep(g);

	vector<pair<int,int> > l;
	g.sample_edges(l, n_edges);
	printf( "Sampled edges = %lld\n", (long long)l.size());

	t = clock();
	long long c0 = 0, c1 = 0, c2 = 0;
	for(long long i = 0; i < (long long)l.size(); ++i) {
		int r = g.delete_edge(l[i].first,l[i].second);
		if(r == 0) ++c0;
		if(r == 1) ++c1;
		if(r == 2) ++c2;
	}
	printf( "Deletion time = %0.3lf sec, c0 = %lld, c1 = %lld, c2 = %lld\n", (clock()-t)*1.0/CLOCKS_PER_SEC, c0, c1, c2);

	t = clock();
	c0 = 0; c1 = 0;
	for(long long i = 0; i < (long long)l.size(); ++i) {
		int r = g.insert_edge(l[i].first,l[i].second);
		if(r == 0) ++c0;
		if(r == 1) ++c1;
	}
	printf( "Insertion time = %0.3lf sec, c0 = %lld, c1 = %lld\n", (clock()-t)*1.0/CLOCKS_PER_SEC, c0, c1);

	print_dep(g);
	print_avg(g);
}

void stream(string path, long long window_size, bool use_union_find = true) {
	clock_t t = clock();
	DynamicCC g(path, false, use_union_find);
	g.init();
	printf( "Initialize time = %0.3lf sec\n", (clock()-t)*1.0/CLOCKS_PER_SEC);

	vector<pair<long long,pair<int,int> > > l;
	int n;
	long long m;

	FILE *fin = fopen( (path+"graph.stream").c_str(), "rb" );
	fread(&n, sizeof(int), 1, fin);
	fread(&m, sizeof(long long), 1, fin);
	printf( "n = %d, m = %lld\n", n, m );

	l.resize(m);
	fread(l.data(),sizeof(pair<long long,pair<int,int> >), m, fin);

	fclose(fin);

	long long ws = window_size;
	if(window_size>0 && window_size<100)
	{
		long long tmin = 99999999999999;
		long long tmax = 0;
		for (long long j = 0; j < (long long)l.size(); ++j)
		{
			tmin = min(tmin, l[j].first);
			tmax = max(tmax, l[j].first);
		}
		double w = double(window_size)/100;
		window_size = (long long)((tmax - tmin) * w);
		cout << "tmin = " << tmin << " tmax = " << tmax << " window_size = " << window_size <<" "<<w<< endl;
	}
	if(window_size < 0) window_size = l[l.size()-1].first;

	t = clock();
	long long c0_ins = 0, c1_ins = 0, c0_del = 0, c1_del = 0, c2_del = 0;
	for(long long s = 0, i = 0; i < (long long) l.size(); ++i) {
		int r = g.insert_edge(l[i].second.first, l[i].second.second);
		if(r == 0) ++c0_ins; else ++c1_ins;
		for(; l[i].first-l[s].first > window_size; ++s) {
			r = g.delete_edge(l[s].second.first, l[s].second.second);
			if(r == 0) ++c0_del;
			else if(r == 1) ++c1_del;
			else if(r == 2) ++c2_del;
		}
	}


	printf( "Update time = %0.3lf sec, c0_ins = %lld, c1_ins = %lld, c0_del = %lld, c1_del = %lld, c2_del = %lld, n_ins = %lld, n_del = %lld\n",
			(clock()-t)*1.0/CLOCKS_PER_SEC, c0_ins, c1_ins, c0_del, c1_del, c2_del, c0_ins+c1_ins, c0_del+c1_del+c2_del);
	print_dep(g);

	if(ws == -2) {
		t = clock();
		for(long long s = 0, i = 0; i < (long long) l.size(); ++i) {
			int r = g.delete_edge(l[i].second.first, l[i].second.second);
			if(r == 0) ++c0_del;
			else if(r == 1) ++c1_del;
			else if(r == 2) ++c2_del;
		}
		printf( "Deletion time = %0.3lf sec, c0_del = %lld, c1_del = %lld, c2_del = %lld, n_del = %lld\n",
					(clock()-t)*1.0/CLOCKS_PER_SEC, c0_del, c1_del, c2_del, c0_del+c1_del+c2_del);
	}
	print_avg(g);
}

void query_random(string path, long long n_queries, bool use_union_find = true) {
	DynamicCC g(path, true, use_union_find);
	clock_t t = clock();
	g.init();
	printf( "Initialize time = %0.3lf sec\n", (clock()-t)*1.0/CLOCKS_PER_SEC);

	t = clock();
	long long c0 = 0, c1 = 0;
	for(long long i = 0; i < n_queries; ++i) {
		int u = rand() % g.n, v = rand() % g.n;
		if(g.query(u,v)) ++c1; else ++c0;
	}
	printf( "Num query = %lld, Query time = %0.3lf sec, Num true = %lld, Num false = %lld\n", n_queries, (clock()-t)*1.0/CLOCKS_PER_SEC, c1, c0);
}

void query_stream(string path, long long window_size, long long n_queries, bool use_union_find = true) {
	DynamicCC g(path,false, use_union_find);
	clock_t t = clock();
	g.init();
	printf( "Initialize time = %0.3lf sec\n", (clock()-t)*1.0/CLOCKS_PER_SEC);

	vector<pair<long long,pair<int,int> > > l;
	int n;
	long long m;

	FILE *fin = fopen( (path+"graph.stream").c_str(), "rb" );
	fread(&n, sizeof(int), 1, fin);
	fread(&m, sizeof(long long), 1, fin);
	printf( "n = %d, m = %lld\n", n, m );

	l.resize(m);
	fread(l.data(),sizeof(pair<long long,pair<int,int> >), m, fin);

	fclose(fin);

	if(window_size>0 && window_size<100)
	{
		long long tmin = 99999999999999;
		long long tmax = 0;
		for (long long j = 0; j < (long long)l.size(); ++j)
		{
			tmin = min(tmin, l[j].first);
			tmax = max(tmax, l[j].first);
		}
		double w = double(window_size)/100;
		window_size = (long long)((tmax - tmin) * w);
		cout << "tmin = " << tmin << " tmax = " << tmax << " window_size = " << window_size <<" "<<w<< endl;
	}

	if(window_size < 0) window_size = l[l.size()-1].first;

	t = clock();

	long long cnt_query = 0, c0 = 0, c1 = 0;

	for(long long s = 0, i = 0; i < (long long) l.size(); ++i) {
		g.insert_edge(l[i].second.first, l[i].second.second);
		for(; l[i].first-l[s].first > window_size; ++s)
			g.delete_edge(l[s].second.first, l[s].second.second);
	}

	t = clock();
	printf( "Start Querying...\n" );
	for(cnt_query = 0; cnt_query < n_queries; ++cnt_query) {
		int u = rand() % g.n, v = rand() % g.n;
		if(g.query(u,v)) ++c1; else ++c0;
	}

	printf( "Num query = %lld,  Time used = %0.3lf sec, Num true = %lld, Num false = %lld\n", cnt_query, (clock()-t)*1.0/CLOCKS_PER_SEC, c1, c0);
}

/*
 * ETW: Function added to this code to parse BinaryFileStreams that include queries.
 */
void bin_query_stream(string path, bool use_union_find = true) {
	BinaryFileStream stream(path);

	size_t vertices = stream.vertices();
	size_t num_ops = stream.edges();
	size_t ops_processed = 0;
	size_t batch_size = 4096;
	DynamicCC g(vertices, use_union_find);

	GraphStreamUpdate ops[batch_size];
	size_t print_freq = 1e7;
	size_t last_print = 0;
	
	clock_t start = clock();
	g.init();
	clock_t query_time = 0;
	while (ops_processed < num_ops) {
		size_t num_ops = stream.get_update_buffer(ops, batch_size);
		for (size_t i = 0; i < num_ops; i++) {
			GraphStreamUpdate op = ops[i];
			if (op.type == INSERT) {
				g.insert_edge(op.edge.src, op.edge.dst);
			} else if (op.type == DELETE) {
				g.delete_edge(op.edge.src, op.edge.dst);
			} else if (op.type == BREAKPOINT) { // streaming utilities doesn't include QUERY = 2?
				// queries in our streams come in batches. Time a bunch at once to reduce clock overhead
				clock_t qt = clock();
				while (ops[i + 1].type == BREAKPOINT && i < num_ops) {
					bool conn = g.query(ops[i].edge.src, ops[i].edge.dst);
#ifdef PrintQuery
					std::cout << ops[i].edge.src << "--" << ops[i].edge.dst << (conn ? " yes" : " no") << std::endl;
#endif
					i++;
				}
				bool conn = g.query(ops[i].edge.src, ops[i].edge.dst);
#ifdef PrintQuery
				std::cout << ops[i].edge.src << "--" << ops[i].edge.dst << (conn ? " yes" : " no") << std::endl;
#endif
				query_time += clock() - qt;
			} else {
				std::cerr << "ERROR: did not recognize update type: " << op.type << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		ops_processed += num_ops;
		if (ops_processed > last_print + print_freq) {
			cout << ops_processed << "\r"; fflush(stdout);
			last_print = ops_processed;
		}
	}
	cout << std::endl;

	clock_t end = clock();

	std::ofstream out("dyn_results.txt");
	out << "Processing: " << path << " took:" << std::endl;
	out << "Insert latency: " << (end - start - query_time) * 1.0 / CLOCKS_PER_SEC << " seconds" << std::endl;
	out << "Query latency: " << query_time * 1.0 / CLOCKS_PER_SEC << " seconds" << std::endl;
}

int main(int argc, char *argv[]) {
	printf( "argc=%d\n", argc );
	for( int i = 0; i < argc; ++i )
		printf( "argv[%d]=%s\n", i, argv[i] );

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	clock_t t = clock();

	if( argc > 1 ) {
		if(strcmp(argv[1], "txt-to-bin") == 0)
			DynamicCC::create_bin( /*dataset*/argv[2] );
		else if( strcmp( argv[1], "sample" ) == 0 )
			sample(argv[2], atoll(argv[3]), argc > 4 ? atoi(argv[4]):1);
		else if( strcmp( argv[1], "query-random" ) == 0 )
			query_random(argv[2], atoll(argv[3]),argc > 4 ? atoi(argv[4]):1);
		else if( strcmp(argv[1], "txt-to-stream" ) == 0 ) {
			bool is_bipartite = false;
			if(argc > 3 && strcmp(argv[3], "bipartite") == 0) is_bipartite = true;
			DynamicCC::create_stream( /*dataset*/argv[2], is_bipartite );
		}
		else if( strcmp( argv[1], "stream" ) == 0 )
			stream(argv[2], atoll(argv[3]), argc > 4 ? atoi(argv[4]):1);
		else if( strcmp( argv[1], "query-stream" ) == 0 )
			query_stream(argv[2], atoll(argv[3]), atoll(argv[4]), argc > 5 ? atoi(argv[5]):1);
		else if (strcmp(argv[1], "binary-file-stream") == 0)
			bin_query_stream(argv[2], argc > 3 ? atoi(argv[3]):true);
		else
			cout << "Did not recognize command: " << argv[2] << std::endl;
	}

	if( argc <= 1) {
		cout << "No command given... doing nothing" << std::endl;
	}

	t = clock() - t;
	std::ofstream out("dyn_results.txt", std::ios::app);
	out << "Total time = " << t*1.0/CLOCKS_PER_SEC << " seconds\n" << endl;
	out << "Memory usage = " << get_max_mem_used() << " MiB" << endl;
	

	return 0;
}

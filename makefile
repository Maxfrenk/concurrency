main.o : lockfree_multimap.hpp threadsafe_hashmap.hpp
	g++ -c lockfree_multimap.hpp threadsafe_hashmap.hpp
check : threadsafe_hashmap.hpp
	g++ -c threadsafe_hashmap.hpp test.cpp

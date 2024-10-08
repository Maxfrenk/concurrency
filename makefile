main.o : lockfree_multimap.hpp threadsafe_hashmap.hpp threadsafe_queue.hpp
	g++ -c lockfree_multimap.hpp threadsafe_hashmap.hpp threadsafe_queue.hpp
check : threadsafe_hashmap.hpp threadsafe_queue.hpp
	g++ -o test test_hashmap.cpp test_queue.cpp

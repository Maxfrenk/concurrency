main.o : lockfree_multimap.hpp threadsafe_hashmap.hpp threadsafe_queue.hpp
	g++ -c lockfree_multimap.hpp threadsafe_hashmap.hpp threadsafe_queue.hpp
check_queue : threadsafe_queue.hpp
	g++ -o test_queue test_queue.cpp
check_hashmap : threadsafe_hashmap.hpp 
	g++ -o test_hashmap test_hashmap.cpp
check_thread_pool : thread_pool.hpp
	g++ -o test_thread_pool test_thread_pool.cpp

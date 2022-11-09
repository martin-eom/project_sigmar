#define BASE

#ifndef EVENTS
#include "events.h"
#endif
#ifndef UNITS
#include "units.h"
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <iostream>
#include <list>
#include <algorithm>
#include <string>
#include <cmath>



template <typename T> class Node {
	public:
		T data;
		Node<T>* next;
		Node<T>* prev;
		Node<T>(T data) {
			this->data = data;
			next = NULL;
			prev = NULL;
		}
};

template <typename T> class LinkedList {
	private:
		int _length;
	public:
		Node<T>* head;
		Node<T>* tail;
		int length() {
			return _length;
		}
		
		LinkedList<T>() {
			head = NULL;
			tail = NULL;
			_length = 0;
		}
		
		void Push(Node<T>* node) {
			if(!head) {
				head = node;
				tail = node;
				node->next = NULL;
				node->prev = NULL;
			}
			else {
				head->prev = node;
				node->next = head;
				head = node;
				node->prev = NULL;
			}
			_length++;
		}
		
		void Append(Node<T>* node) {
			if(!head) {
				head = node;
				tail = node;
				node->next = NULL;
				node->prev = NULL;
			}
			else {
				Node<T>* current = head;
				while(current->next) {
					current = current->next;
				}
				current->next = node;
				node->prev = current;
				node->next = NULL;
				tail = node;
			}
			_length++;
		}
		
		void Insert(int i, Node<T>* node) {
			if(i == 0) {
				if(head) {
					node->next = head;
					head->prev = node;
					head = node;
					node->prev = NULL;
				}
				else {
					node->next = NULL;
					node->prev = NULL;
					head = node;
					tail = node;
				}
				_length++;
			}
			else {
				if(head) {
					int ind = 0;
					Node<T>* current = head;
					while(ind + 1 < i) {
						if(current->next) {
							current = current->next;
							ind++;
						}
						else {
							std::cout << "ERROR: LinkedList: List index out of range: " << ind << " " << i << "\n";
							break;
						}
					}
					if(i == ind + 1) {
						node->next = current->next;
						node->prev = current;
						current->next = node;
						if(node->next) {
							node->next->prev = node;
						}
						else {
							tail = node;
						}
						_length++;
					}
				}
				else {
					std::cout << "ERROR: LinkedList: List index out of range: " << 0 << " " << i << "\n";										
				}
			}
			
		}
		
		void InsertSorted(Node<T>* node) {
			if(head) {
				if(node->data < head->data) {
					node->next = head;
					head->prev = node;
					head = node;
					node->prev = NULL;
				}
				else {
					Node<T>* before = head;
					Node<T>* after = head->next;
					while(after) {
						if(node->data < after->data) {
							break;
						}
						before = after;
						after = after->next;
					}
					node->next = after;
					node->prev = before;
					before->next = node;
					if(after) {
						after->prev = node;
					}
					else {
						tail = node;
					}
				}
			}
			else {
				head = node;
				tail = node;
				node->next = NULL;
				node->prev = NULL;
			}
			_length++;
		}
		
		Node<T>* Pop(int i) {
			bool outOfRange = false;
			Node<T>* current = head;
			if(i == 0) {
				if(head) {
					head = head->next;
					if(head) {
						head->prev = NULL;
					}
				}
				else {
					std::cout << "ERROR: LinkedList: List index out of range: " << -1 << " " << i << "\n";
					outOfRange = true;
				}
			}
			else {
				int ind = 0;
				while(ind + 1 < i) {
					if(current->next) {
						current = current->next;
						ind++;
					}
					else {
						std::cout << "ERROR: LinkedList: List index out of range: " << ind + 1 << " " << i << "\n";
						outOfRange = true;
						break;
					}
				}
				current = current->next;
			}
			if(!outOfRange) {
				if(current->next) {
					if(current->prev) {
						current->prev->next = current->next;
					}
					else {
						head = current->next;
					}
					current->next->prev = current->prev;
				}
				else {
					if(current->prev) {
						current->prev->next = NULL;
					}
					else {
						head = NULL;
					}
					tail = current->prev;
				}
				current->next = NULL;
				current->prev = NULL;
				_length--;
			}
			return current;
		}

		T get(int i) {
			int ind = 0;
			Node<T>* current = head;
			while(current) {
				if(ind < i) {
					current = current->next;
					ind++;
				}
				else break;
			}
			return current->data;
		}
		
		void Empty() {
			Node<T>* current = tail;
			Node<T>* prev;
			while(current) {
				prev = current->prev;
				delete current;
				current = prev;
			}
			head = NULL;
			tail = NULL;
			_length = 0;
		}
		
		void InsertionSort() {
			LinkedList<T> sorted;
			while(head) {
				Node<T>* node = Pop(0);
				sorted.InsertSorted(node);
			}
			head = sorted.head;
			tail = sorted.tail;
		}

		void print() {
			Node<T>* current = head;
			while(current) {
				std::cout << current->data << " ";
				current = current->next;
			}
			std::cout << std::endl;
		}
};

class Listener;


class EventManager {
	public:
		std::list<int> noPrint = {GENERIC_EVENT, TICK_EVENT, SDL_EVENT};
		std::list<Listener*> listeners;
		int fps = 30;
		double dt = 1./30.;
		
		EventManager() {};
		EventManager(int fps){
			this->fps = fps;
			dt = 1./fps;
		}
				
		void RegisterListener(Listener* listener) {
			listeners.push_back(listener);
		}
		
		void UnregisterListener(Listener* listener) {
			listeners.remove(listener);
		}
		
		void Post(Event* ev);

};


class Listener {
	public:
		EventManager* em;
		
		Listener(EventManager* em) {
			this->em = em;
			em->RegisterListener(this);
		}
		
		virtual void Notify(Event* ev){}
};


void EventManager::Post(Event* ev) {
	if(std::find(noPrint.begin(), noPrint.end(), ev->type)==noPrint.end()) {
		std::cout << " - posted " << ev->name << "\n";
	}
	for (std::list<Listener*>::iterator it = listeners.begin();
		it != listeners.end(); it++) {
			(*it)->Notify(ev);
		}
}

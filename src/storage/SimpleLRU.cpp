#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


void SimpleLRU::del_last_node(){


      lru_node* tail_node = _tail;
      if (tail_node == nullptr) {return;}

      std::size_t node_size = tail_node->key.size() + tail_node->value.size();
      _cur_size -= node_size;

      _lru_index.erase(tail_node->key);
      if(_head.get() != _tail) {
        _tail = _tail->prev;
        _tail->next.reset(nullptr);
      }
      else {_head.reset(nullptr);}
}


void SimpleLRU::insert_new_node(lru_node *node) {
      if(_head.get()) { _head.get()->prev = node;}
      else {_tail = node;}
      node->next = std::move(_head);
      _head.reset(node);
}

void SimpleLRU::move_node_to_head(lru_node *node){
      if(_head.get() == node) {return;}

      if(node->next.get()) {
          auto tmp_head = std::move(_head);
          _head = std::move(node->prev->next);
          node->prev->next = std::move(node->next);
          tmp_head.get()->prev = node;
          node->next = std::move(tmp_head);
          node->prev->next->prev = node->prev;
        }
      else{
        _tail = node->prev;
        _head.get()->prev = node;
        node->next = std::move(_head);
        _head = std::move(node->prev->next);
      }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
      std::size_t node_size = key.size() + value.size();
      if(node_size > _max_size) {return false;}

      auto node = _lru_index.find(key);
      if(node != _lru_index.end()) {
        std::size_t delta = value.size() - (node->second).get().value.size() ? value.size() - (node->second).get().value.size():0;
        move_node_to_head(&node->second.get());
        while(_cur_size + delta > _max_size) {del_last_node();}
        node->second.get().value = value;
        _cur_size += delta;
        }
      else {

          while(_cur_size + node_size > _max_size) {del_last_node();}
          lru_node* new_node = new lru_node{key, value, nullptr, nullptr};
          insert_new_node(new_node);
          _lru_index.insert({std::reference_wrapper<const std::string>(new_node->key), std::reference_wrapper<lru_node>(*new_node)});
          _cur_size += node_size;
        }
      return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
      std::size_t node_size = key.size() + value.size();
      if(node_size > _max_size) {return false;}

      auto node = _lru_index.find(key);
      if(node != _lru_index.end()) {return false;}
      else {
          while(_cur_size + node_size > _max_size) {del_last_node();}
          lru_node* new_node = new lru_node{key, value, nullptr, nullptr};
          insert_new_node(new_node);
          _lru_index.insert({std::reference_wrapper<const std::string>(new_node->key), std::reference_wrapper<lru_node>(*new_node)});
          _cur_size += node_size;
          return true;
      }
 }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
      if(key.size() + value.size() > _max_size) {return false;}

      auto node = _lru_index.find(key);
      if(node == _lru_index.end()) {return false;}

      std::size_t delta = value.size() > (node->second).get().value.size()? value.size() - (node->second).get().value.size():0;
      move_node_to_head(&node->second.get());
      while(_cur_size + delta > _max_size) {del_last_node();}
      node->second.get().value = value;
      _cur_size += delta;
      return true;
}

// See MapBasedGlobalLockImpl.h

bool SimpleLRU::Delete(const std::string &key) {
      auto it_node = _lru_index.find(key);
      if(it_node == _lru_index.end()) {return false;}

      lru_node* node = &(it_node->second.get());
      _lru_index.erase(node->key);
      std::size_t node_size = node->key.size() + node->value.size();
      _cur_size -= node_size;

      if(node == _head.get() && !(node->next.get())) {
        _head.reset();
        _tail = nullptr;
      }

      if (node!=_head.get()&& !(node->next.get())) {
        _tail = node->prev;
        node->prev->next.reset();
      }

      if(node == _head.get() && node->next.get()) {
        node->next.get()->prev = nullptr;
        _head = std::move(node->next);
      }

      if (node!=_head.get()&& node->next.get()) {
        node->next.get()->prev = node->prev;
        node->prev->next = std::move(node->next);
      }

      return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
      auto node = _lru_index.find(key);
      if(node == _lru_index.end()) { return false;}

      value = node->second.get().value;
      move_node_to_head(&node->second.get());
      return true;
}

} // namespace Backend
} // namespace Afina

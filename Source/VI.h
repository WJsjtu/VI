#pragma once

#ifdef _WIN32
#ifdef VI_DLL
#define VI_PORT __declspec(dllexport)
#else
#define VI_PORT __declspec(dllimport)
#endif
#else
#if __has_attribute(visibility)
#define VI_PORT __attribute__((visibility("default")))
#else
#define VI_PORT
#endif
#endif

#include <assert.h>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace VI {

template <typename T>
class LinkedList {
private:
    class ListNode {
    private:
        T _value;
        ListNode *_next, *_prev;
        friend LinkedList<T>;

    public:
        ListNode() : _next(nullptr), _prev(nullptr) {}
        ListNode(const ListNode& node) : _next(node._next), _prev(node._prev), _value(node._value) {}
        ListNode(ListNode&& node) : _next(node._next), _prev(node._prev), _value(node._value) { node._next = node._prev = nullptr; }
        ListNode(const T& value) : _value(value), _next(nullptr), _prev(nullptr) {}
        ListNode(T&& value) : _value(value), _next(nullptr), _prev(nullptr) {}
        ListNode& operator=(const T& value) {
            _value = value;
            return *this;
        }
        ListNode& operator=(const ListNode& element) {
            _value = element._value;
            return *this;
        }
        ListNode& operator=(const ListNode* element) {
            _value = element->_value;
            return *this;
        }
    };
    ListNode *_head, *_tail;
    size_t _size;

public:
    class ListNodeException {
    public:
        ListNodeException() = delete;
        ListNodeException(const char* message) {
            size_t size = strlen(message);
            _message = new char[size + 1];
            _message[size] = '\0';
            memcpy(_message, message, size);
        }
        ListNodeException(const ListNodeException& error) {
            size_t size = strlen(error._message);
            _message = new char[size + 1];
            _message[size] = '\0';
            memcpy(_message, error._message, size);
        }
        ListNodeException(ListNodeException&& error) {
            _message = error._message;
            error._message = NULL;
        }
        ~ListNodeException() {
            if (_message) {
                delete[] _message;
                _message = NULL;
            }
        }
        const char* what() { return _message; }

    private:
        char* _message = NULL;
    };
    LinkedList() : _head(nullptr), _tail(nullptr), _size(0) {}
    LinkedList(const LinkedList<T>& list) : _head(nullptr), _tail(nullptr), _size(0) {
        for (size_t i = 0; i < list.size(); i++) {
            add(list[i]);
        }
    }
    LinkedList(LinkedList<T>&& list) : _head(list._head), _tail(list._tail), _size(list._size) {
        list._head = list._tail = nullptr;
        list._size = 0;
    }
    LinkedList& operator=(const LinkedList<T>& list) {
        clear();
        for (size_t i = 0; i < list.size(); i++) {
            add(list[i]);
        }
        return *this;
    }
    ~LinkedList() {
        clear();
        delete _head;
        delete _tail;
        _head = _tail = nullptr;
    }
    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }
    void add(const T& value) {
        ListNode* newElement = new ListNode(value);
        if (_size != 0) {
            newElement->_prev = _tail;
            _tail->_next = newElement;
            _tail = newElement;
        } else {
            _head = _tail = newElement;
        }
        ++_size;
    }
    void add(size_t index, const T& value) {
        if (_size == 0) {
            add(value);
            return;
        }
        if (index >= 0 && index < _size) {
            ListNode* node = _head;
            for (size_t i = 0; i < index; ++i) node = node->_next;
            ListNode* newElement = new ListNode(value);
            newElement->_next = node;
            newElement->_prev = node->_prev;
            if (node->_prev != nullptr) node->_prev->_next = newElement;
            node->_prev = newElement;
            if (index == 0) _head = newElement;
            if (index == _size - 1) _tail = newElement->_next;
            ++_size;
        } else {
            throw ListNodeException("LinkedList :: add(index, value)");
        }
    }
    T remove(size_t index) {
        if (index >= 0 && index < _size) {
            ListNode* node = _head;
            for (size_t i = 0; i < index; ++i) node = node->_next;
            if (node->_prev != nullptr) node->_prev->_next = node->_next;
            if (node->_next != nullptr) node->_next->_prev = node->_prev;
            if (_tail == node && node->_prev != nullptr) _tail = node->_prev;
            if (_head == node && node->_next != nullptr) _head = node->_next;
            if (_head == _tail && _tail == node) _head = _tail = nullptr;
            node->_next = nullptr;
            node->_prev = nullptr;
            --_size;
            return node->_value;
        } else {
            throw ListNodeException("LinkedList :: remove(index)");
        }
    }
    void clear() {
        if (_size != 0) {
            ListNode* node = _tail;
            while (node != nullptr) {
                if (node->_next != nullptr) {
                    delete node->_next;
                    node->_next = nullptr;
                }
                if (node->_prev != nullptr)
                    node = node->_prev;
                else {
                    delete node;
                    node = nullptr;
                }
            }
            _head = _tail = nullptr;
            _size = 0;
        }
    }
    T get(size_t index) const {
        if (index >= 0 && index < _size) {
            ListNode* node = _head;
            for (size_t i = 0; i < index; ++i) node = node->_next;
            return node->_value;
        } else {
            throw ListNodeException("LinkedList :: get(index)");
        }
    }
    T set(size_t index, const T& value) {
        if (index >= 0 && index < _size) {
            ListNode* node = _head;
            for (size_t i = 0; i < index; ++i) node = node->_next;
            T tmp = node->_value;
            node->_value = value;
            return tmp;
        } else {
            throw ListNodeException("LinkedList :: set(index, value)");
        }
    }
    bool swap(size_t index1, size_t index2) {
        if (index1 >= 0 && index1 < _size && index2 >= 0 && index2 < _size) {
            ListNode *node1, *node2;
            {
                ListNode* node = _head;
                for (size_t i = 0; i < index1; ++i) node = node->_next;
                node1 = node;
            }
            {
                ListNode* node = _head;
                for (size_t i = 0; i < index2; ++i) node = node->_next;
                node2 = node;
            }
            T tmp = node1->_value;
            node1->_value = node2->_value;
            node2->_value = tmp;
            return true;
        } else
            return false;
    }
    T& operator[](size_t index) const {
        if (index >= 0 && index < _size) {
            ListNode* node = _head;
            for (size_t i = 0; i < index; ++i) node = node->_next;
            return node->_value;
        } else {
            throw ListNodeException("LinkedList :: operator [index]");
        }
    }
};

class VI_PORT String {
public:
    String();
    String(const char* str, size_t size = 0);
    String(const String& str);
    ~String();
    String& operator=(const String& str);
    bool operator==(const char* str);
    bool operator==(const String& str);
    const char* data() const;
    const size_t size() const;

private:
    char* m_data;
    size_t m_size;
};

template class VI_PORT LinkedList<String>;

class VI_PORT Camera {
public:
    Camera(int deviceID = 0);
    Camera(int deviceID, int width, int height, int fps = 0);
    ~Camera();

    static LinkedList<String> getDeviceList();

    // Tells you when a new frame has arrived - you should call this if you have
    // specified setAutoReconnectOnFreeze to true
    bool isFrameNew();

    bool isDeviceSetup();

    // Or pass in a buffer for getPixels to fill returns true if successful.
    bool getPixels(unsigned char* pixels, bool rgb = false, bool flipX = true, bool flipY = false);

    // Launches a pop up settings window
    // For some reason in GLUT you have to call it twice each time.
    void showSettingsWindow();

    // get width, height and number of pixels
    int getWidth();
    int getHeight();

private:
    void* m_handle;
};

class VI_PORT Video {
public:
    Video(const String& file);
    ~Video();

    int64_t getFramesCount();
    double getTotalTime();
    double getFPS();
    int64_t getBitRate();

    int64_t getCurrentFrame();
    double getCurrentTime();

    int getWidth();
    int getHeight();

    void seekFrame(int64_t frame_number);
    void seekTime(double sec);

    bool retrieveFrame(double sec, unsigned char** data, bool rgb = false);

private:
    void* m_handle;
};

enum class VI_PORT LogLevel { Debug = 1, Info = 2, Warning = 3, Error = 4 };

struct VI_PORT Logger {
public:
    void virtual onMessage(LogLevel, const char*){};
};

void VI_PORT SetGlobalLogger(Logger* logger);
}  // namespace VI

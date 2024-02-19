#ifndef JSON_HPP
#define JSON_HPP

#include <iostream>
#include <concepts>
#include <string>
#include <string_view>
#include <sstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>
#include <type_traits>
#include <optional>
#include <initializer_list>
#include <utility>

namespace json {

template <typename T, typename... others>
concept type_is_one_of = (std::same_as<T, others> or ...);

template <typename T>
concept is_json_leaf_type = type_is_one_of<T, float, int, std::string, bool>;

class Malformed : public std::exception {
public:
    const char* what() const throw () override;
};

class ValueType {
public:
    enum class Concrete {
        Object, List, String, Float, Int, Bool, Null
    };
    static constexpr std::string toString(Concrete c) {
        switch (c) {
            case Concrete::Object:
                return "Object";
            case Concrete::List:
                return "List";
            case Concrete::String:
                return "String";
            case Concrete::Float:
                return "Float";
            case Concrete::Int:
                return "Int";
            case Concrete::Bool:
                return "Bool";
            default:
                return "null";
        }
    }
    template <is_json_leaf_type T>
    constexpr static Concrete get() {
        if(typeid(T) == typeid(std::string))
            return Concrete::String;
        if(typeid(T) == typeid(float))
            return Concrete::Float;
        if(typeid(T) == typeid(int))
            return Concrete::Int;
        if(typeid(T) == typeid(bool))
            return Concrete::Bool;
        return Concrete::Null;
    }
};

class WrongObjectType : public std::exception {
private:
    std::string msg;
public:
    WrongObjectType(std::string msg);
    const char* what() const throw() override;
    static WrongObjectType NotObject();
    static WrongObjectType NotList();
    template <is_json_leaf_type T>
    static WrongObjectType NotLeaf() {
        return WrongObjectType(std::string("This is not leaf type ") + ValueType::toString(ValueType::get<T>()) + ".");
    }
    static WrongObjectType Unknown();
};

class Node {
private:
    const ValueType::Concrete type_;
protected:
    Node(ValueType::Concrete type);
public:
    virtual ~Node();
    ValueType::Concrete type() const;
    virtual std::string pretty() const = 0;
    virtual std::ostream& dump(std::ostream& os) const = 0;
    bool isNull() const;
};

class NullNode : public Node {
public:
    NullNode();
    std::string pretty() const override;
    std::ostream& dump(std::ostream& os) const override;
};

class ObjectNode : public Node {
private:
    std::map<std::string, std::shared_ptr<Node>> children;
public:
    ObjectNode() : Node(ValueType::Concrete::Object) { }
    void addOrEditChild(std::string key, std::shared_ptr<Node> child);
    std::map<std::string, std::shared_ptr<Node>>& getChildren();
    std::string pretty() const override;
    std::ostream& dump(std::ostream& os) const override;
};

template <is_json_leaf_type T>
class ValueNode : public Node {
private:
    T value_;
public:
    ValueNode(T value) : Node(ValueType::get<T>()), value_(value) { }
    T value() const { return value_; }
    ValueNode& operator=(T value) {
        value_ = value;
        return *this;
    }
    std::string pretty() const override {
        std::ostringstream os;
        os << value_;
        return os.str();
    }
    std::ostream& dump(std::ostream& os) const override {
        return os << value_;
    }
};

class ListNode : public Node {
private:
    std::vector<std::shared_ptr<Node>> children;
public:
    ListNode();
    void addChild(std::shared_ptr<Node> child);
    std::vector<std::shared_ptr<Node>>& getChildren();
    std::string pretty() const override;
    std::ostream& dump(std::ostream& os) const override;
    std::shared_ptr<Node> get(size_t idx);
};

template <typename T>
concept is_json_container_type = type_is_one_of<T, ListNode, ObjectNode>;

template <typename T>
concept is_json_container_or_base_type = type_is_one_of<T, ListNode, ObjectNode, Node>;

template <typename T>
concept is_json_valid_type = type_is_one_of<T, ListNode, ObjectNode, NullNode, Node>;

// Parsing
static std::shared_ptr<Node>        parse(std::string& json);
static std::shared_ptr<Node>        parseRecursively(std::string& json, size_t& index);
static std::shared_ptr<ObjectNode>  parseObject(std::string& json, size_t& index);
static std::shared_ptr<ListNode>    parseList(std::string& json, size_t& index);
static std::shared_ptr<Node>        parseValue(std::string& json, size_t& index);

class Json {
public:
    class View {
    public:
        typedef std::shared_ptr<std::shared_ptr<Node>> ppNode;
    private:
        ppNode node;
    public:
        template <is_json_valid_type T>
        View(std::shared_ptr<T>& node) {
            this->node = std::make_shared<std::shared_ptr<Node>>(node);
        }
        View operator[] (std::string key);
        View operator[] (size_t idx);
        template <is_json_leaf_type T>
        View operator=(T value) {
            *std::static_pointer_cast<ValueNode<T>>(*node) = value;
            return *this;
        }
        template <is_json_leaf_type T>
        T as() {
            return std::static_pointer_cast<ValueNode<T>>(*node)->value();
        }
    };
private:
    std::shared_ptr<Node> root;
    Json(std::shared_ptr<Node> root);
public:
    Json(ValueType::Concrete type);
    Json(std::initializer_list<std::pair<std::string, Json>> list);
    template <is_json_leaf_type T>
    Json(T value) : root(std::make_shared<ValueNode<T>>(value)) { }
    Json(const char* value);
    static Json fromFile(const std::string filename);
    static Json array(std::initializer_list<Json>& list);
    std::ostream& dump(std::ostream& os) const;
public:
    View operator[] (std::string key);
    View operator[] (size_t idx);
    template <is_json_leaf_type T>
    T as() {
        return std::static_pointer_cast<ValueNode<T>>(root)->value();
    }
public:
    friend std::ostream& operator<<(std::ostream& os, const Json& json) {
        return json.dump(os);
    }
};

Json fromFile(const std::string filename);
Json array(std::initializer_list<Json> list);

};

#endif

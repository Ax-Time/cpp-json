#include "json.hpp"

#include <sstream>
#include <fstream>

namespace json {

const char* Malformed::what() const throw () {
    return "Malformed json.";
}

WrongObjectType::WrongObjectType(std::string msg) : msg(msg) { }
const char* WrongObjectType::what() const throw() {
    return msg.c_str();
}

WrongObjectType WrongObjectType::NotObject() {
    return WrongObjectType("This is not a json object.");
}
WrongObjectType WrongObjectType::NotList() {
    return WrongObjectType("This is not a list object.");
}

WrongObjectType WrongObjectType::Unknown() {
    return WrongObjectType("Unknown object type.");
}

Node::Node(ValueType::Concrete type) : type_(type) { }

Node::~Node() { }

bool Node::isNull() const {
    return type_ == ValueType::Concrete::Null;
}

ValueType::Concrete Node::type() const { return type_; }

NullNode::NullNode() : Node(ValueType::Concrete::Null) { }

std::string NullNode::pretty() const { return "null"; }

std::ostream& NullNode::dump(std::ostream& os) const {
    return os << "null";
}

void ObjectNode::addOrEditChild(std::string key, std::shared_ptr<Node> child) { 
    children[key] = std::move(child);
}

std::string ObjectNode::pretty() const {
    std::ostringstream os;
    os << "{ ";
    for (auto [key, value] : children) {
        os << " " << "\"" << key << "\"" << " : " << value->pretty() << ", ";
    }
    os << " }";
    return os.str();
}

std::ostream& ObjectNode::dump(std::ostream& os) const {
    os << "{";
    for (auto it = children.begin(); it != children.end(); ++it) {
        auto [key, value] = *it;
        os << '"' << key << '"' << ":";
        value->dump(os);
        os << (std::next(it) == children.end() ? "" : ",");
    }
    os << "}";
    return os;
}

std::map<std::string, std::shared_ptr<Node>>& ObjectNode::getChildren() {
    return children;
}

ListNode::ListNode() : Node(ValueType::Concrete::List) { }

void ListNode::addChild(std::shared_ptr<Node> child) { children.push_back(child); }

std::string ListNode::pretty() const {
    std::ostringstream os;
    os << "[ ";
    for (auto& child : children) {
        os << child->pretty() << ", ";
    }
    os << "]";
    return os.str();
}

std::ostream& ListNode::dump(std::ostream& os) const {
    os << "[";
    for (auto it = children.begin(); it != children.end(); ++it) {
        it->get()->dump(os);
        os << (std::next(it) == children.end() ? "" : ",");
    }
    os << "]";
    return os;
}

std::shared_ptr<Node> ListNode::get(size_t idx) {
    return children[idx];
}

std::vector<std::shared_ptr<Node>>& ListNode::getChildren() {
    return children;
}

template<>
std::string ValueNode<bool>::pretty() const {
    return value_ ? "true" : "false";
}

template<>
std::string ValueNode<std::string>::pretty() const {
    return "\"" + value_ + "\"";
}

template<>
std::ostream& ValueNode<bool>::dump(std::ostream& os) const {
    return os << (value_ ? "true" : "false");
}

template<>
std::ostream& ValueNode<std::string>::dump(std::ostream& os) const {
    return os << "\"" << value_ << "\"";
}

std::shared_ptr<Node> parse(std::string& str) {
    std::ostringstream oss;

    // Remove whitespace, but not inside strings
    bool inside = false;
    for (char ch : str) {
        if (ch == '"') inside = !inside;
        if (std::isspace(ch) && !inside) continue;
        oss << ch;
    }

    std::string str_clean = oss.str();

    size_t index = 0;
    return parseRecursively(str_clean, index);
}

std::shared_ptr<Node> parseRecursively(std::string& json, size_t& index) {
    switch (json[index]) {
        case '{':
            return std::dynamic_pointer_cast<Node, ObjectNode>(parseObject(json, index));
        case '[':
            return std::dynamic_pointer_cast<Node, ListNode>(parseList(json, index));
        default:
            return parseValue(json, index);
    }
}

std::shared_ptr<ObjectNode> parseObject(std::string& json, size_t& index) {
    ++index; // Skip opening curly brace

    // Create node
    auto node = std::make_shared<ObjectNode>();

    while (json[index] != '}') {
        // Parse key
        std::ostringstream key;
        if (json[index++] != '"') throw Malformed();
        while (json[index] != '"') {
            key << json[index++];
        }
        ++index; // Skip final quote
        ++index; // Skip colon

        // Parse value
        std::shared_ptr<Node> value = parseRecursively(json, index);

        if (json[index] != '}') ++index; // Skip comma

        // Add pair
        node->addOrEditChild(key.str(), value);
    }

    ++index; // Skip closing curly brace

    return node;
}

std::shared_ptr<ListNode> parseList(std::string& json, size_t& index) {
    ++index; // Skip opening square bracket

    // Create node
    auto node = std::make_shared<ListNode>();

    while (json[index] != ']') {
        node->addChild(parseRecursively(json, index));
        if (json[index] != ']') ++index; // Skip comma
    }

    ++index; // Skip closing square bracket

    return node;
}

std::shared_ptr<Node> parseValue(std::string& json, size_t& index) {
    // Bool
    if (json.substr(index, 4) == "true") {
        index += 4;
        return std::shared_ptr<Node>((Node*)(new ValueNode<bool>(true)));
    }
    if (json.substr(index, 5) == "false") {
        index += 5;
        return std::shared_ptr<Node>((Node*)(new ValueNode<bool>(false)));
    }
    // Number
    auto is_number = [](char ch) { return '0' <= ch && ch <= '9'; };
    if (is_number(json[index])) {
        std::ostringstream number;
        bool is_float = false;
        while(json[index] == '.' || is_number(json[index])) {
            if (json[index] == '.') is_float = true;
            number << json[index++];
        }
        if (is_float) return std::shared_ptr<Node>((Node*)(new ValueNode<float>(std::atof(number.str().c_str()))));
        return std::shared_ptr<Node>((Node*)(new ValueNode<int>(std::atoi(number.str().c_str()))));
    }
    // String
    if (json[index++] == '"') {
        std::ostringstream string;
        while (json[index] != '"') {
            string << json[index++];
        }
        ++index; // Skip last quote
        return std::shared_ptr<Node>((Node*)(new ValueNode<std::string>(string.str())));
    }
    throw new Malformed();
}

Json::Json(std::shared_ptr<Node> root) : root(root) { }

Json::Json(const char* value) : Json(std::string(value)) { }

Json Json::fromFile(const std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("File not found.");
    std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parse(str);
}

Json Json::array(std::initializer_list<Json>& list) {
    auto node = std::make_shared<ListNode>();
    for (auto& value : list) {
        node->addChild(value.root);
    }
    return Json(node);
}

std::ostream& Json::dump(std::ostream& os) const {
    return root->dump(os);
}

Json::Json(std::initializer_list<std::pair<std::string, Json>> list) {
    auto node = std::make_shared<ObjectNode>();
    for (auto [key, value] : list) {
        node->addOrEditChild(key, value.root);
    }
    root = node;
}

Json::View Json::operator[] (std::string key) {
    if(root->type() != ValueType::Concrete::Object)
        throw WrongObjectType::NotObject();
    auto object = std::dynamic_pointer_cast<ObjectNode>(root);
    auto it = object->getChildren().find(key);
    if(it == object->getChildren().end()) {
        auto newNode = std::make_shared<NullNode>();
        object->addOrEditChild(key, newNode);
        return View(newNode);
    }
    return View(it->second);
}

Json::View Json::operator[] (size_t idx) {
    if (root->type() != ValueType::Concrete::List)
        throw WrongObjectType::NotList();
    auto list = std::dynamic_pointer_cast<ListNode>(root);
    return View(list->getChildren()[idx]);
}

Json::View Json::View::operator[] (std::string key) {
    if((*node)->type() != ValueType::Concrete::Object)
        throw WrongObjectType::NotObject();
    auto object = std::dynamic_pointer_cast<ObjectNode>(*node);
    auto it = object->getChildren().find(key);
    if(it == object->getChildren().end()) {
        auto newNode = std::make_shared<NullNode>();
        object->addOrEditChild(key, newNode);
        return View(newNode);
    }
    return View(it->second);
}

Json::View Json::View::operator[] (size_t idx) {
    if((*node)->type() != ValueType::Concrete::List)
        throw WrongObjectType::NotList();
    auto list = std::dynamic_pointer_cast<ListNode>(*node);
    return View(list->getChildren()[idx]);
}

Json fromFile(const std::string filename) {
    return Json::fromFile(filename);
}

Json array(std::initializer_list<Json> list) {
    return Json::array(list);
}

}; // namespace json

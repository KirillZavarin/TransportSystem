#include "json_builder.h"

using namespace std::string_literals;

namespace json {

	Builder::BuilderContext::BuilderContext(Builder& builder) : builder_(builder) {
	}


	Builder::DictItemContext& Builder::BuilderContext::StartDict() {
		return builder_.StartDict();
	}

	Builder::KeyItemContext& Builder::BuilderContext::Key(const std::string& key) {
		return builder_.Key(key);
	}

	Builder::DictItemContext& Builder::KeyItemContext::Value(const Node::Value& val) {
		return builder_.Value(val).dict_item_context;
	}

	Builder& Builder::BuilderContext::EndDict() {
		return builder_.EndDict();
	}

	Builder::ArrayItemContext& Builder::BuilderContext::StartArray() {
		return builder_.StartArray();
	}

	Builder::ArrayItemContext& Builder::ArrayItemContext::Value(const Node::Value& val) {
		return builder_.Value(val).array_item_context;
	}

	Builder& Builder::BuilderContext::EndArray() {
		return builder_.EndArray();
	}

	Builder& Builder::Value(const Node::Value& val) {

		if (!root_.IsNull()) {
			throw std::logic_error("Calling Value when the object is ready"s);
		}

		Node node = Node();
		InitializationNode(node, val);

		if (nodes_stack_.empty()) {
			root_ = node;
			return *this;
		}

		if (nodes_stack_.back()->IsArray()) {
			Array arr = Array(nodes_stack_.back()->AsArray().begin(), nodes_stack_.back()->AsArray().end());
			arr.emplace_back(node);
			delete nodes_stack_.back();
			Node* arr_node_ptr = new Node(arr);
			nodes_stack_.back() = arr_node_ptr;
			return *this;
		}

		if (nodes_stack_.back()->IsString()) {
			std::string key = nodes_stack_.back()->AsString();
			delete nodes_stack_.back();
			nodes_stack_.pop_back();
			Dict map = Dict(nodes_stack_.back()->AsDict().begin(), nodes_stack_.back()->AsDict().end());
			map.emplace(key, node);
			delete nodes_stack_.back();
			Node* dict_node_ptr = new Node(map);
			nodes_stack_.back() = dict_node_ptr;
			return *this;
		}
		throw std::logic_error("Calling Value anywhere except after the constructor, after the Key, or after the previous element of the array"s);
	}

	Builder::KeyItemContext& Builder::Key(const std::string& key) {
		if (nodes_stack_.empty() || !root_.IsNull()) {
			throw std::logic_error("Calling Key when the object is ready"s);
		}

		if (!nodes_stack_.back()->IsDict()) {
			throw std::logic_error("Calling the Key method outside the dictionary or immediately after another Key"s);
		}

		Node* key_node_ptr = new Node(key);
		nodes_stack_.push_back(key_node_ptr);
		return key_item_context;
	}

	Builder::ArrayItemContext& Builder::StartArray() {
		if (!root_.IsNull()) {
			throw std::logic_error("Calling StartArray when the object is ready."s);
		}

		if (!(nodes_stack_.empty() || nodes_stack_.back()->IsArray() ||
			nodes_stack_.back()->IsString())) {
			throw std::logic_error("Calling StartArray anywhere except after the constructor, after the Key, or after the previous element of the array"s);
		}

		Node* arr_node_ptr = new Node(Array());
		nodes_stack_.push_back(arr_node_ptr);
		return array_item_context;
	}

	Builder& Builder::EndArray() {
		if (nodes_stack_.size() == 1u) {
			if (!nodes_stack_.back()->IsArray()) {
				throw std::logic_error("Calling EndArray in the context of another container."s);
			}
			root_ = *nodes_stack_.back();
			nodes_stack_.pop_back();
			return *this;
		}

		if (nodes_stack_.size() > 1u) {
			if (!nodes_stack_.back()->IsArray()) {
				throw std::logic_error("Calling EndArray in the context of another container."s);
			}
			Node* arr_node_ptr = nodes_stack_.back();
			nodes_stack_.pop_back();
			Value(arr_node_ptr->GetValue());
			delete arr_node_ptr;
			return *this;
		}

		throw std::logic_error("Calling EndArray when the object is ready."s);
	}

	Builder::DictItemContext& Builder::StartDict() {
		if (!root_.IsNull()) {
			throw std::logic_error("Calling StartDict when the object is ready."s);
		}

		if (!(nodes_stack_.empty() || nodes_stack_.back()->IsArray() ||
			nodes_stack_.back()->IsString())) {
			throw std::logic_error("Calling StartDict anywhere except after the constructor, after the Key, or after the previous element of the array"s);
		}

		Node* dict_node_ptr = new Node(Dict());
		nodes_stack_.push_back(dict_node_ptr);
		return dict_item_context;
	}

	Builder& Builder::EndDict() {
		if (nodes_stack_.size() == 1) {
			if (!nodes_stack_.back()->IsDict()) {
				throw std::logic_error("Calling EndDict in the context of another container."s);
			}
			root_ = *nodes_stack_.back();
			nodes_stack_.pop_back();
			return *this;
		}

		if (nodes_stack_.size() > 1) {
			if (!nodes_stack_.back()->IsDict()) {
				throw std::logic_error("Calling EndDict in the context of another container."s);
			}
			Node* dict_node_ptr = nodes_stack_.back();
			nodes_stack_.pop_back();
			Value(dict_node_ptr->GetValue());
			delete dict_node_ptr;
			return *this;
		}

		throw std::logic_error("Calling EndDict when the object is ready."s);
	}

	Node Builder::Build() {
		if (root_.IsNull()) {
			throw std::logic_error("Root is null"s);
		}

		if (!nodes_stack_.empty()) {
			throw std::logic_error("Stack is not empty"s);
		}

		return root_;
	}

	void Builder::InitializationNode(Node& node, const Node::Value& value) {
		size_t index = value.index();
		switch (index) {
		case 0: node = Node(std::get<0>(value));
			break;
		case 1: node = Node(std::get<1>(value));
			break;
		case 2: node = Node(std::get<2>(value));
			break;
		case 3: node = Node(std::get<3>(value));
			break;
		case 4: node = Node(std::get<4>(value));
			break;
		case 5: node = Node(std::get<5>(value));
			break;
		case 6: node = Node(std::get<6>(value));
			break;
		}
	}
}
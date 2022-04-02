#pragma once
#include <vector>
#include <algorithm>

#include "json.h"


namespace json {

	class Builder {

		class BuilderContext;
		class DictItemContext;
		class KeyItemContext;
		class ArrayItemContext;

		class BuilderContext {
		public:
			BuilderContext(Builder& builder);
		public:
			DictItemContext& StartDict();

			KeyItemContext& Key(const std::string& key);

			Builder& EndDict();

			ArrayItemContext& StartArray();

			Builder& EndArray();
		protected:
			Builder& builder_;
		};

		class DictItemContext final : public BuilderContext {
			ArrayItemContext& StartArray() = delete;
			DictItemContext& StartDict() = delete;
			Builder& EndArray() = delete;
		};

		class KeyItemContext final : public BuilderContext {
		public:
			Builder& EndArray() = delete;
			Builder& EndDict() = delete;
			KeyItemContext& Key(const std::string& key) = delete;
			DictItemContext& Value(const Node::Value& val);
		};

		class ArrayItemContext final : public BuilderContext {
		public:
			Builder& EndDict() = delete;
			KeyItemContext& Key(const std::string& key) = delete;
			ArrayItemContext& Value(const Node::Value& val);
		};
	public:
		Builder& Value(const Node::Value& val);

		KeyItemContext& Key(const std::string& key);

		ArrayItemContext& StartArray();

		Builder& EndArray();

		DictItemContext& StartDict();

		Builder& EndDict();

		Node Build();
	private:
		Node root_ = nullptr;
		std::vector<Node*> nodes_stack_;
		KeyItemContext key_item_context{ *this };
		DictItemContext dict_item_context{ *this };
		ArrayItemContext array_item_context{ *this };

		void InitializationNode(Node& node, const Node::Value& value);
	};
}
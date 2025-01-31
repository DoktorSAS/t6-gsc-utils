#include <stdinc.hpp>
#include "array.hpp"
#include "execution.hpp"

namespace scripting
{
	array_value::array_value(unsigned int parent_id, unsigned int id)
		: id_(id)
		, parent_id_(parent_id)
	{
		if (!this->id_)
		{
			return;
		}

		const auto value = game::scr_VarGlob->childVariableValue[this->id_];
		game::VariableValue variable;
		variable.u = value.u.u;
		variable.type = (game::scriptType_e)value.type;

		this->value_ = variable;
	}

	void array_value::operator=(const script_value& _value)
	{
		if (!this->id_)
		{
			return;
		}

		const auto value = _value.get_raw();

		const auto variable = &game::scr_VarGlob->childVariableValue[this->id_];
		game::VariableValue variable_{};
		variable_.type = variable->type;
		variable_.u = variable->u.u;

		game::AddRefToValue(game::SCRIPTINSTANCE_SERVER, &value);
		game::RemoveRefToValue(game::SCRIPTINSTANCE_SERVER, variable_.type, variable_.u);

		variable->type = gsl::narrow_cast<char>(value.type);
		variable->u.u = value.u;

		this->value_ = value;
	}

	array::array(const unsigned int id)
		: id_(id)
	{
		this->add();
	}

	array::array(const array& other)
	{
		this->operator=(other);
	}

	array::array(array&& other) noexcept
	{
		this->id_ = other.id_;
		other.id_ = 0;
	}

	array::array()
	{
		this->id_ = make_array();
	}
	
	array::array(std::vector<script_value> values)
	{
		this->id_ = make_array();

		for (const auto& value : values)
		{
			this->push(value);
		}
	}

	array::array(std::unordered_map<std::string, script_value> values)
	{
		this->id_ = make_array();

		for (const auto& value : values)
		{
			this->set(value.first, value.second);
		}
	}

	array::~array()
	{
		this->release();
	}

	array& array::operator=(const array& other)
	{
		if (&other != this)
		{
			this->release();
			this->id_ = other.id_;
			this->add();
		}

		return *this;
	}

	array& array::operator=(array&& other) noexcept
	{
		if (&other != this)
		{
			this->release();
			this->id_ = other.id_;
			other.id_ = 0;
		}

		return *this;
	}

	void array::add() const
	{
		if (this->id_)
		{
			game::VariableValue value{};
			value.u.uintValue = this->id_;
			value.type = game::SCRIPT_OBJECT;

			game::AddRefToValue(game::SCRIPTINSTANCE_SERVER, &value);
		}
	}

	void array::release() const
	{
		if (this->id_)
		{
			game::VariableValue value{};
			value.u.uintValue = this->id_;
			value.type = game::SCRIPT_OBJECT;

			game::RemoveRefToValue(game::SCRIPTINSTANCE_SERVER, value.type, value.u);
		}
	}

	std::vector<script_value> array::get_keys() const
	{
		std::vector<script_value> result;

		auto current = game::scr_VarGlob->objectVariableChildren[this->id_].firstChild;

		while (current)
		{
			const auto var = game::scr_VarGlob->childVariableValue[current];

			if (var.type == game::SCRIPT_NONE)
			{
				current = var.nextSibling;
				continue;
			}

			const auto string_value = (unsigned __int8)var.name_lo + (var.k.keys.name_hi << 8);
			const auto* str = game::SL_ConvertToString(string_value);

			script_value key;
			if (string_value < 0x40000 && str)
			{
				key = str;
			}
			else
			{
				key = (string_value - 0x800000) & 0xFFFFFF;
			}

			result.push_back(key);

			current = var.nextSibling;
		}

		return result;
	}

	unsigned int array::size() const
	{
		return game::Scr_GetSelf(game::SCRIPTINSTANCE_SERVER, this->id_);
	}

	unsigned int array::push(script_value value) const
	{
		this->set(this->size(), value);
		return this->size();
	}

	void array::erase(const unsigned int index) const
	{
		const auto variable_id = game::FindVariable(game::SCRIPTINSTANCE_SERVER, this->id_, (index - 0x800000) & 0xFFFFFF);
		if (variable_id)
		{
			game::RemoveVariableValue(game::SCRIPTINSTANCE_SERVER, this->id_, variable_id);
		}
	}

	void array::erase(const std::string& key) const
	{
		const auto string_value = game::SL_GetString(key.data(), 0);
		const auto variable_id = game::FindVariable(game::SCRIPTINSTANCE_SERVER, this->id_, string_value);
		if (variable_id)
		{
			game::RemoveVariableValue(game::SCRIPTINSTANCE_SERVER, this->id_, variable_id);
		}
	}

	script_value array::pop() const
	{
		const auto value = this->get(this->size() - 1);
		this->erase(this->size() - 1);
		return value;
	}

	script_value array::get(const std::string& key) const
	{
		const auto string_value = game::SL_GetString(key.data(), 0);
		const auto variable_id = game::FindVariable(game::SCRIPTINSTANCE_SERVER, this->id_, string_value);

		if (!variable_id)
		{
			return {};
		}

		const auto value = game::scr_VarGlob->childVariableValue[variable_id];
		game::VariableValue variable;
		variable.u = value.u.u;
		variable.type = (game::scriptType_e)value.type;

		return variable;
	}

	script_value array::get(const unsigned int index) const
	{
		const auto variable_id = game::FindVariable(game::SCRIPTINSTANCE_SERVER, this->id_, (index - 0x800000) & 0xFFFFFF);

		if (!variable_id)
		{
			return {};
		}

		const auto value = game::scr_VarGlob->childVariableValue[variable_id];
		game::VariableValue variable;
		variable.u = value.u.u;
		variable.type = (game::scriptType_e)value.type;

		return variable;
	}

	script_value array::get(const script_value& key) const
	{
		if (key.is<int>())
		{
			this->get(key.as<int>());
		}

		if (key.is<std::string>())
		{
			this->get(key.as<std::string>());
		}

		return {};
	}

	void array::set(const std::string& key, const script_value& _value) const
	{
		const auto value = _value.get_raw();

		const auto string_value = game::SL_GetString(key.data(), 0);
		const auto variable_id = this->get_value_id(key);

		if (!variable_id)
		{
			return;
		}

		const auto variable = &game::scr_VarGlob->childVariableValue[variable_id];
		game::VariableValue variable_{};
		variable_.type = variable->type;
		variable_.u = variable->u.u;

		game::AddRefToValue(game::SCRIPTINSTANCE_SERVER, &value);
		game::RemoveRefToValue(game::SCRIPTINSTANCE_SERVER, variable_.type, variable_.u);

		variable->type = gsl::narrow_cast<char>(value.type);
		variable->u.u = value.u;
	}

	void array::set(const unsigned int index, const script_value& _value) const
	{
		const auto value = _value.get_raw();
		const auto variable_id = this->get_value_id(index);

		if (!variable_id)
		{
			return;
		}

		const auto variable = &game::scr_VarGlob->childVariableValue[variable_id];
		game::VariableValue variable_{};
		variable_.type = variable->type;
		variable_.u = variable->u.u;

		game::AddRefToValue(game::SCRIPTINSTANCE_SERVER, &value);
		game::RemoveRefToValue(game::SCRIPTINSTANCE_SERVER, variable_.type, variable_.u);

		variable->type = gsl::narrow_cast<char>(value.type);
		variable->u.u = value.u;
	}

	void array::set(const script_value& key, const script_value& _value) const
	{
		if (key.is<int>())
		{
			this->set(key.as<int>(), _value);
		}

		if (key.is<std::string>())
		{
			this->set(key.as<std::string>(), _value);
		}
	}

	unsigned int array::get_entity_id() const
	{
		return this->id_;
	}

	unsigned int array::get_value_id(const std::string& key) const
	{
		const auto string_value = game::SL_GetString(key.data(), 0);
		const auto variable_id = game::FindVariable(game::SCRIPTINSTANCE_SERVER, this->id_, string_value);

		if (!variable_id)
		{
			return game::GetNewVariable(game::SCRIPTINSTANCE_SERVER, this->id_, string_value);
		}

		return variable_id;
	}

	unsigned int array::get_value_id(const unsigned int index) const
	{
		const auto variable_id = game::FindVariable(game::SCRIPTINSTANCE_SERVER, this->id_, (index - 0x800000) & 0xFFFFFF);
		if (!variable_id)
		{
			return game::GetNewArrayVariable(game::SCRIPTINSTANCE_SERVER, this->id_, index);
		}

		return variable_id;
	}

	entity array::get_raw() const
	{
		return entity(this->id_);
	}
}

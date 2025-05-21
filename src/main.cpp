namespace REX::JSON
{
	using list_t = std::vector<std::string>;

	namespace Impl
	{
		template <>
		void SettingLoad<list_t>(
			void* a_data,
			path_t a_path,
			list_t& a_value,
			list_t& a_valueDefault)
		{
			const auto& json = *static_cast<nlohmann::json*>(a_data);
			if (a_path[0] == '/')
			{
				a_value = json.value<list_t>(nlohmann::json::json_pointer(a_path.data()), a_valueDefault);
			}
			else
			{
				a_value = json.value<list_t>(a_path, a_valueDefault);
			}
		}

		template <>
		void SettingSave<list_t>(
			void* a_data,
			path_t a_path,
			list_t& a_value)
		{
			auto& json = *static_cast<nlohmann::json*>(a_data);
			if (a_path[0] == '/')
			{
				json[nlohmann::json::json_pointer(a_path.data())] = a_value;
			}
			else
			{
				json[a_path] = a_value;
			}
		}
	}

	template <class Store = SettingStore>
	using StrA = Setting<list_t, Store>;
}

namespace JSON
{
	static REX::JSON::Bool AutoInteriors{ "autoInteriors", true };
	static REX::JSON::Bool AutoSmallWorld{ "autoSmallWorld", true };
	static REX::JSON::StrA AutoSmallWorldBlockList{ "autoSmallWorldBlocklist", {} };
	static REX::JSON::StrA AutoWorldSpaces{
		"autoWorldSpaces",
		{ "SETheFringe"s,
		  "SETheFringeOrdered"s,
		  "SENSBliss"s,
		  "SENSCrucible"s,
		  "SENSPalace"s }
	};

	static void Init()
	{
		const auto json = REX::JSON::SettingStore::GetSingleton();
		json->Init(
			"OBSE/plugins/BakaAutoOpenLocalMap.json",
			"OBSE/plugins/BakaAutoOpenLocalMapCustom.json");
		json->Load();
	}
}

namespace Hooks
{
	static bool ListHasWorldSpace(const std::vector<std::string>& a_list, const std::string a_name)
	{
		const auto it = std::find_if(
			a_list.begin(),
			a_list.end(),
			[&a_name](const std::string& a_iter)
			{
				if (a_name.size() != a_iter.size())
				{
					return false;
				}
				return std::equal(
					a_name.cbegin(),
					a_name.cend(),
					a_iter.cbegin(),
					a_iter.cend(),
					[](auto a_name, auto a_iter)
					{
						return std::toupper(a_name) == std::toupper(a_iter);
					});
			});
		return (it != a_list.end());
	}

	class hkMapMenu :
		public REX::Singleton<hkMapMenu>
	{
	private:
		static void SwitchTabsNotifyingAltar(void* a_this, std::int32_t a_id, void* a_target)
		{
			using func_t = decltype(&hkMapMenu::SwitchTabsNotifyingAltar);
			static REL::Relocation<func_t> func{ REL::Offset(0x658CE30) };
			return func(a_this, a_id, a_target);
		}

		static void StartFadeIn(void* a_this)
		{
			_StartFadeIn(a_this);
			
			if (auto Player = RE::PlayerCharacter::GetSingleton())
			{
				if (JSON::AutoInteriors && Player->GetInterior())
				{
					SwitchTabsNotifyingAltar(a_this, 1, nullptr);
					return;
				}

				if (auto WorldSpace = Player->GetWorldSpace())
				{
					auto editorID = WorldSpace->editorID.c_str();
					if (JSON::AutoSmallWorld && WorldSpace->flags & 1 &&
					    !ListHasWorldSpace(JSON::AutoSmallWorldBlockList.GetValue(), editorID))
					{
						SwitchTabsNotifyingAltar(a_this, 1, nullptr);
						return;
					}

					if (ListHasWorldSpace(JSON::AutoWorldSpaces.GetValue(), editorID))
					{
						SwitchTabsNotifyingAltar(a_this, 1, nullptr);
						return;
					}
				}
			}
		}

		inline static REL::HookVFT _StartFadeIn{ RE::MapMenu::VTABLE[0], 24, StartFadeIn };
	};
}

OBSE_PLUGIN_LOAD(const OBSE::LoadInterface* a_obse)
{
	OBSE::Init(a_obse);
	JSON::Init();
	return true;
}

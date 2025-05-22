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
	static REX::JSON::Bool DisableFogOfWar{ "disableFogOfWar", false };

	static REX::JSON::Bool RefocusWorldMap{ "refocusWorldMap", true };
	static REX::JSON::I32 RefocusWorldMapDelay{ "refocusWorldMapDelay", 30 };

	static REX::JSON::StrA AutoSmallWorldBlockList{
		"autoSmallWorldBlocklist",
		{}
	};

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

namespace HOOK
{
	class hkMapMenu :
		public REX::Singleton<hkMapMenu>
	{
	private:
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

		static void SwitchTabs(RE::MapMenu* a_this)
		{
			a_this->SwitchTabsNotifyingAltar(1, nullptr);
			iFrame = std::min<std::int32_t>(std::max<std::int32_t>(0, JSON::RefocusWorldMapDelay), 60);
		}

		static void DoIdle(RE::MapMenu* a_this)
		{
			_DoIdle(a_this);

			if (auto InterfaceManager = RE::InterfaceManager::GetInstance(false, true);
			    InterfaceManager && InterfaceManager->mapPageNumber != 2)
			{
				return;
			}

			if (iFrame > 0)
			{
				iFrame--;
				if (iFrame == 0)
				{
					if (a_this->worldMapPlayerArrow)
					{
						auto x = a_this->worldMapPlayerArrow->GetFloat(4015);
						auto y = a_this->worldMapPlayerArrow->GetFloat(4016);
						a_this->CenterMapAt(x, y, true);
					}
				}
			}
		}

		static void StartFadeOut(RE::MapMenu* a_this)
		{
			_StartFadeOut(a_this);

			iFrame = 0;
		}

		static void StartFadeIn(RE::MapMenu* a_this)
		{
			_StartFadeIn(a_this);

			if (auto InterfaceManager = RE::InterfaceManager::GetInstance(false, true);
			    InterfaceManager && InterfaceManager->mapPageNumber != 2)
			{
				return;
			}

			if (auto Player = RE::PlayerCharacter::GetSingleton())
			{
				if (JSON::AutoInteriors && Player->GetInterior())
				{
					SwitchTabs(a_this);
					return;
				}

				if (auto WorldSpace = Player->GetWorldSpace())
				{
					auto editorID = WorldSpace->editorID.c_str();
					if (JSON::AutoSmallWorld && WorldSpace->flags & 1)
					{
						if (!ListHasWorldSpace(JSON::AutoSmallWorldBlockList.GetValue(), editorID))
						{
							return;
						}

						SwitchTabs(a_this);
						return;
					}

					if (ListHasWorldSpace(JSON::AutoWorldSpaces.GetValue(), editorID))
					{
						SwitchTabs(a_this);
						return;
					}
				}
			}
		}

		inline static std::int32_t iFrame{ 0 };

		inline static REL::HookVFT _DoIdle{ RE::MapMenu::VTABLE[0], 0x12, DoIdle };
		inline static REL::HookVFT _StartFadeOut{ RE::MapMenu::VTABLE[0], 0x16, StartFadeOut };
		inline static REL::HookVFT _StartFadeIn{ RE::MapMenu::VTABLE[0], 0x18, StartFadeIn };

	public:
		inline static REL::Relocation<bool*> FogOfWar{ REL::Offset(0x8FDCC80) };
	};

	static void Init()
	{
		if (JSON::DisableFogOfWar)
		{
			*hkMapMenu::FogOfWar = false;
		}
	}
}

namespace
{
	void MessageHandler(OBSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type)
		{
		case OBSE::MessagingInterface::kPostLoad:
			JSON::Init();
			HOOK::Init();
			break;
		default:
			break;
		}
	}
}

OBSE_PLUGIN_LOAD(const OBSE::LoadInterface* a_obse)
{
	OBSE::Init(a_obse);
	return true;
}

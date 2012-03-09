/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
 *
 * 该 program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * 该 program is distributed在 the hope that it will be useful,
 * 但 WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should 包含 received a copy of the GNU General Public License
 * along with 该 program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ScriptMgr.h"
#include "Policies/SingletonImp.h"
#include "Log.h"
#include "ProgressBar.h"
#include "ObjectMgr.h"
#include "WaypointManager.h"
#include "World.h"

#include "revision_nr.h"

ScriptMapMap sQuestEndScripts;
ScriptMapMap sQuestStartScripts;
ScriptMapMap sSpellScripts;
ScriptMapMap sGameObjectScripts;
ScriptMapMap sEventScripts;
ScriptMapMap sGossipScripts;
ScriptMapMap sCreatureMovementScripts;

INSTANTIATE_SINGLETON_1(ScriptMgr);

ScriptMgr::ScriptMgr() :
    m_hScriptLib(NULL),
    
    m_scheduledScripts(0),

    m_pOnInitScriptLibrary(NULL),
    m_pOnFreeScriptLibrary(NULL),
    m_pGetScriptLibraryVersion(NULL),

    m_pGetCreatureAI(NULL),
    m_pCreateInstanceData(NULL),

    m_pOnGossipHello(NULL),
    m_pOnGOGossipHello(NULL),
    m_pOnGossipSelect(NULL),
    m_pOnGOGossipSelect(NULL),
    m_pOnGossipSelectWithCode(NULL),
    m_pOnGOGossipSelectWithCode(NULL),
    m_pOnQuestAccept(NULL),
    m_pOnGOQuestAccept(NULL),
    m_pOnItemQuestAccept(NULL),
    m_pOnQuestRewarded(NULL),
    m_pOnGOQuestRewarded(NULL),
    m_pGetNPCDialogStatus(NULL),
    m_pGetGODialogStatus(NULL),
    m_pOnGOUse(NULL),
    m_pOnItemUse(NULL),
    m_pOnGossipSelectItem(NULL),
    m_pOnAreaTrigger(NULL),
    m_pOnProcessEvent(NULL),
    m_pOnEffectDummyCreature(NULL),
    m_pOnEffectDummyGO(NULL),
    m_pOnEffectDummyItem(NULL),
    m_pOnAuraDummy(NULL)
{
}

ScriptMgr::~ScriptMgr()
{
    UnloadScriptLibrary();
}

void ScriptMgr::LoadScripts(ScriptMapMap& scripts, const char* tablename)
{
    if (IsScriptScheduled())                                // function don't must be called在 time scripts use.
        return;

    sLog.outString("%s :", tablename);

    scripts.clear();                                        // need for reload support

    QueryResult *result = WorldDatabase.PQuery("SELECT id, delay, command, datalong, datalong2, datalong3, datalong4, data_flags, dataint, dataint2, dataint3, dataint4, x, y, z, o FROM %s", tablename);

    uint32 count = 0;

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> 加载了 %u 个 脚本", count);
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();

        Field *fields = result->Fetch();
        ScriptInfo tmp;
        tmp.id          = fields[0].GetUInt32();
        tmp.delay       = fields[1].GetUInt32();
        tmp.command     = fields[2].GetUInt32();
        tmp.raw.data[0] = fields[3].GetUInt32();
        tmp.raw.data[1] = fields[4].GetUInt32();
        tmp.raw.data[2] = fields[5].GetUInt32();
        tmp.raw.data[3] = fields[6].GetUInt32();
        tmp.raw.data[4] = fields[7].GetUInt32();
        tmp.raw.data[5] = fields[8].GetInt32();
        tmp.raw.data[6] = fields[9].GetInt32();
        tmp.raw.data[7] = fields[10].GetInt32();
        tmp.raw.data[8] = fields[11].GetInt32();
        tmp.x           = fields[12].GetFloat();
        tmp.y           = fields[13].GetFloat();
        tmp.z           = fields[14].GetFloat();
        tmp.o           = fields[15].GetFloat();

        // generic command args check
        switch(tmp.command)
        {
            case SCRIPT_COMMAND_TALK:
            {
                if (tmp.talk.chatType > CHAT_TYPE_ZONE_YELL)
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 CHAT_TYPE_ (datalong = %u)在 SCRIPT_COMMAND_TALK 中，脚本 ID:  %u", tablename, tmp.talk.chatType, tmp.id);
                    continue;
                }
                if (tmp.talk.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.talk.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_TALK 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.talk.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.talk.creatureEntry && !tmp.talk.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_TALK 中，脚本 ID:  %u, 但 值太小 (datalong3 = %u).", tablename, tmp.talk.creatureEntry, tmp.id, tmp.talk.searchRadius);
                    continue;
                }
                if (!GetLanguageDescByID(tmp.talk.language))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong4 = %u在 SCRIPT_COMMAND_TALK 中，脚本 ID:  %u, 但 该 语言 不存在", tablename, tmp.talk.language, tmp.id);
                    continue;
                }
                if (tmp.talk.textId[0] == 0)
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 talk text id (dataint = %i)在 SCRIPT_COMMAND_TALK 中，脚本 ID:  %u", tablename, tmp.talk.textId[0], tmp.id);
                    continue;
                }

                for(int i = 0; i < MAX_TEXT_ID; ++i)
                {
                    if (tmp.talk.textId[i] && (tmp.talk.textId[i] < MIN_DB_SCRIPT_STRING_ID || tmp.talk.textId[i] >= MAX_DB_SCRIPT_STRING_ID))
                    {
                        sLog.outErrorDb("数据表 `%s` 包含 不合法的 text id (dataint = %i expected %u-%u)在 SCRIPT_COMMAND_TALK 中，脚本 ID:  %u", tablename, tmp.talk.textId[i], MIN_DB_SCRIPT_STRING_ID, MAX_DB_SCRIPT_STRING_ID, tmp.id);
                        continue;
                    }
                }

                // if (!GetMangosStringLocale(tmp.dataint)) will be checked after db_script_string loading
                break;
            }
            case SCRIPT_COMMAND_EMOTE:
            {
                if (!sEmotesStore.LookupEntry(tmp.emote.emoteId))
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 emote id (datalong = %u)在 SCRIPT_COMMAND_EMOTE 中，脚本 ID:  %u", tablename, tmp.emote.emoteId, tmp.id);
                    continue;
                }
                if (tmp.emote.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.emote.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_EMOTE 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.emote.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.emote.creatureEntry && !tmp.emote.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_EMOTE 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong3 = %u).", tablename, tmp.emote.creatureEntry, tmp.id, tmp.emote.searchRadius);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_TELEPORT_TO:
            {
                if (!sMapStore.LookupEntry(tmp.teleportTo.mapId))
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 map (Id: %u)在 SCRIPT_COMMAND_TELEPORT_TO 中，脚本 ID:  %u", tablename, tmp.teleportTo.mapId, tmp.id);
                    continue;
                }

                if (!MaNGOS::IsValidMapCoord(tmp.x, tmp.y, tmp.z, tmp.o))
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 coordinates (X: %f Y: %f)在 SCRIPT_COMMAND_TELEPORT_TO 中，脚本 ID:  %u", tablename, tmp.x, tmp.y, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_QUEST_EXPLORED:
            {
                Quest const* quest = sObjectMgr.GetQuestTemplate(tmp.questExplored.questId);
                if (!quest)
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 quest (ID: %u)在 SCRIPT_COMMAND_QUEST_EXPLORED在 `datalong` 中，脚本 ID:  %u", tablename, tmp.questExplored.questId, tmp.id);
                    continue;
                }

                if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 quest (ID: %u)在 SCRIPT_COMMAND_QUEST_EXPLORED在 `datalong` 中，脚本 ID:  %u, 但 quest 没有标记 QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT在 quest flags. Script command or quest flags wrong. Quest modified to require objective.", tablename, tmp.questExplored.questId, tmp.id);

                    // 该 will prevent quest completing without objective
                    const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT);

                    // continue; - quest objective requirement set and command can be allowed
                }

                if (float(tmp.questExplored.distance) > DEFAULT_VISIBILITY_DISTANCE)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 过大的值 (%u) for exploring objective complete在 `datalong2`在 SCRIPT_COMMAND_QUEST_EXPLORED在 `datalong` 中，脚本 ID:  %u",
                        tablename, tmp.questExplored.distance, tmp.id);
                    continue;
                }

                if (tmp.questExplored.distance && float(tmp.questExplored.distance) > DEFAULT_VISIBILITY_DISTANCE)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 过大的值 (%u) for exploring objective complete在 `datalong2`在 SCRIPT_COMMAND_QUEST_EXPLORED在 `datalong` 中，脚本 ID:  %u, max distance is %f or 0 for disable distance check",
                        tablename, tmp.questExplored.distance, tmp.id, DEFAULT_VISIBILITY_DISTANCE);
                    continue;
                }

                if (tmp.questExplored.distance && float(tmp.questExplored.distance) < INTERACTION_DISTANCE)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 过小的值 (%u) for exploring objective complete在 `datalong2`在 SCRIPT_COMMAND_QUEST_EXPLORED在 `datalong` 中，脚本 ID:  %u, min distance is %f or 0 for disable distance check",
                        tablename, tmp.questExplored.distance, tmp.id, INTERACTION_DISTANCE);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_KILL_CREDIT:
            {
                if (!ObjectMgr::GetCreatureTemplate(tmp.killCredit.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 creature (Entry: %u)在 SCRIPT_COMMAND_KILL_CREDIT 中，脚本 ID:  %u", tablename, tmp.killCredit.creatureEntry, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:
            {
                GameObjectData const* data = sObjectMgr.GetGOData(tmp.GetGOGuid());
                if (!data)
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 gameobject (GUID: %u)在 SCRIPT_COMMAND_RESPAWN_GAMEOBJECT 中，脚本 ID:  %u", tablename, tmp.GetGOGuid(), tmp.id);
                    continue;
                }

                GameObjectInfo const* info = ObjectMgr::GetGameObjectInfo(data->id);
                if (!info)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 gameobject with错误的 entry (GUID: %u Entry: %u)在 SCRIPT_COMMAND_RESPAWN_GAMEOBJECT 中，脚本 ID:  %u", tablename, tmp.GetGOGuid(), data->id, tmp.id);
                    continue;
                }

                if (info->type == GAMEOBJECT_TYPE_FISHINGNODE ||
                    info->type == GAMEOBJECT_TYPE_FISHINGHOLE ||
                    info->type == GAMEOBJECT_TYPE_DOOR        ||
                    info->type == GAMEOBJECT_TYPE_BUTTON      ||
                    info->type == GAMEOBJECT_TYPE_TRAP)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 gameobject 类型 (%u) unsupported by command SCRIPT_COMMAND_RESPAWN_GAMEOBJECT 中，脚本 ID:  %u", tablename, info->id, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
            {
                if (!MaNGOS::IsValidMapCoord(tmp.x, tmp.y, tmp.z, tmp.o))
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 coordinates (X: %f Y: %f)在 SCRIPT_COMMAND_TEMP_SUMMON_CREATURE 中，脚本 ID:  %u", tablename, tmp.x, tmp.y, tmp.id);
                    continue;
                }

                if (!ObjectMgr::GetCreatureTemplate(tmp.summonCreature.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 creature (Entry: %u)在 SCRIPT_COMMAND_TEMP_SUMMON_CREATURE 中，脚本 ID:  %u", tablename, tmp.summonCreature.creatureEntry, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_OPEN_DOOR:
            case SCRIPT_COMMAND_CLOSE_DOOR:
            {
                GameObjectData const* data = sObjectMgr.GetGOData(tmp.GetGOGuid());
                if (!data)
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 gameobject (GUID: %u)在 %s 中，脚本 ID:  %u", tablename, tmp.GetGOGuid(), (tmp.command == SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"), tmp.id);
                    continue;
                }

                GameObjectInfo const* info = ObjectMgr::GetGameObjectInfo(data->id);
                if (!info)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 gameobject with错误的 entry (GUID: %u Entry: %u)在 %s 中，脚本 ID:  %u", tablename, tmp.GetGOGuid(), data->id, (tmp.command == SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"), tmp.id);
                    continue;
                }

                if (info->type != GAMEOBJECT_TYPE_DOOR)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 gameobject 类型 (%u) non supported by command %s 中，脚本 ID:  %u", tablename, info->id, (tmp.command == SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"), tmp.id);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_REMOVE_AURA:
            {
                if (!sSpellStore.LookupEntry(tmp.removeAura.spellId))
                {
                    sLog.outErrorDb("数据表 `%s` 使用了不存在的技能 (id: %u)在 SCRIPT_COMMAND_REMOVE_AURA or SCRIPT_COMMAND_CAST_SPELL 中，脚本 ID:  %u",
                        tablename, tmp.removeAura.spellId, tmp.id);
                    continue;
                }
                if (tmp.removeAura.isSourceTarget & ~0x1)   // 1 bits (0,1)
                {
                    sLog.outErrorDb("数据表 `%s` 使用了未知的标记在 datalong2 (%u)i n SCRIPT_COMMAND_CAST_SPELL 中，脚本 ID:  %u",
                        tablename, tmp.removeAura.isSourceTarget, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_CAST_SPELL:
            {
                if (!sSpellStore.LookupEntry(tmp.castSpell.spellId))
                {
                    sLog.outErrorDb("数据表 `%s` 使用了不存在的技能 (id: %u)在 SCRIPT_COMMAND_REMOVE_AURA or SCRIPT_COMMAND_CAST_SPELL 中，脚本 ID:  %u",
                        tablename, tmp.castSpell.spellId, tmp.id);
                    continue;
                }
                if (tmp.castSpell.flags & ~0x7)             // 3 bits
                {
                    sLog.outErrorDb("数据表 `%s` 使用了未知的标记在 datalong2 (%u)i n SCRIPT_COMMAND_CAST_SPELL 中，脚本 ID:  %u",
                        tablename, tmp.castSpell.flags, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_CREATE_ITEM:
            {
                if (!ObjectMgr::GetItemPrototype(tmp.createItem.itemEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 不存在的物品 (entry: %u)在 SCRIPT_COMMAND_CREATE_ITEM 中，脚本 ID:  %u",
                        tablename, tmp.createItem.itemEntry, tmp.id);
                    continue;
                }
                if (!tmp.createItem.amount)
                {
                    sLog.outErrorDb("数据表 `%s` SCRIPT_COMMAND_CREATE_ITEM 但 amount is %u 中，脚本 ID:  %u",
                        tablename, tmp.createItem.amount, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_DESPAWN_SELF:
            {
                // for later, we might consider despawn by database guid, and define在 datalong2 as option to despawn self.
                break;
            }
            case SCRIPT_COMMAND_PLAY_MOVIE:
            {
                if (!sMovieStore.LookupEntry(tmp.playMovie.movieId))
                {
                    sLog.outErrorDb("数据表 `%s` 使用了不存在的 movie_id (id: %u)在 SCRIPT_COMMAND_PLAY_MOVIE 中，脚本 ID:  %u",
                        tablename, tmp.playMovie.movieId, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_MOVEMENT:
            {
                if (tmp.movement.movementType >= MAX_DB_MOTION_TYPE)
                {
                    sLog.outErrorDb("数据表 `%s` SCRIPT_COMMAND_MOVEMENT 包含错误的 MovementType %u 中，脚本 ID:  %u",
                        tablename, tmp.movement.movementType, tmp.id);
                    continue;
                }
                if (tmp.movement.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.movement.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MOVEMENT 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.movement.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.movement.creatureEntry && !tmp.movement.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MOVEMENT 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong3 = %u).", tablename, tmp.movement.creatureEntry, tmp.id, tmp.movement.searchRadius);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_SET_ACTIVEOBJECT:
            {
                if (tmp.activeObject.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.activeObject.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_SET_ACTIVEOBJECT 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.activeObject.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.activeObject.creatureEntry && !tmp.activeObject.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_SET_ACTIVEOBJECT 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong3 = %u).", tablename, tmp.activeObject.creatureEntry, tmp.id, tmp.activeObject.searchRadius);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_SET_FACTION:
            {
                if (tmp.faction.factionId && !sFactionTemplateStore.LookupEntry(tmp.faction.factionId))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong = %u在 SCRIPT_COMMAND_SET_FACTION 中，脚本 ID:  %u, 但 该 faction-template 不存在", tablename, tmp.faction.factionId, tmp.id);
                    continue;
                }

                if (tmp.faction.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.faction.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_SET_FACTION 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.faction.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.faction.creatureEntry && !tmp.faction.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_SET_FACTION 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong3 = %u).", tablename, tmp.faction.creatureEntry, tmp.id, tmp.faction.searchRadius);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL:
            {
                if (tmp.morph.flags & 0x01)
                {
                    if (tmp.morph.creatureOrModelEntry && !sCreatureDisplayInfoStore.LookupEntry(tmp.morph.creatureOrModelEntry))
                    {
                        sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL 中，脚本 ID:  %u, 但 该 model 不存在", tablename, tmp.morph.creatureOrModelEntry, tmp.id);
                        continue;
                    }
                }
                else
                {
                    if (tmp.morph.creatureOrModelEntry && !ObjectMgr::GetCreatureTemplate(tmp.morph.creatureOrModelEntry))
                    {
                        sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.morph.creatureOrModelEntry, tmp.id);
                        continue;
                    }
                }

                if (tmp.morph.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.morph.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.morph.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.morph.creatureEntry && !tmp.morph.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong3 = %u).", tablename, tmp.morph.creatureEntry, tmp.id, tmp.morph.searchRadius);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL:
            {
                if (tmp.mount.flags & 0x01)
                {
                    if (tmp.mount.creatureOrModelEntry && !sCreatureDisplayInfoStore.LookupEntry(tmp.mount.creatureOrModelEntry))
                    {
                        sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL 中，脚本 ID:  %u, 但 该 model 不存在", tablename, tmp.mount.creatureOrModelEntry, tmp.id);
                        continue;
                    }
                }
                else
                {
                    if (tmp.mount.creatureOrModelEntry && !ObjectMgr::GetCreatureTemplate(tmp.mount.creatureOrModelEntry))
                    {
                        sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.mount.creatureOrModelEntry, tmp.id);
                        continue;
                    }
                }

                if (tmp.mount.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.mount.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.mount.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.mount.creatureEntry && !tmp.mount.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong3 = %u).", tablename, tmp.mount.creatureEntry, tmp.id, tmp.mount.searchRadius);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_SET_RUN:
            {
                if (tmp.run.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.run.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_SET_RUN 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.run.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.run.creatureEntry && !tmp.run.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_SET_RUN 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong3 = %u).", tablename, tmp.run.creatureEntry, tmp.id, tmp.run.searchRadius);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_ATTACK_START:
            {
                if (tmp.attack.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.attack.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_ATTACK_START 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.attack.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.attack.creatureEntry && !tmp.attack.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_ATTACK_START 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong3 = %u).", tablename, tmp.attack.creatureEntry, tmp.id, tmp.attack.searchRadius);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_GO_LOCK_STATE:
            {
                if (!ObjectMgr::GetGameObjectInfo(tmp.goLockState.goEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_GO_LOCK_STATE 中，脚本 ID:  %u, 但 该 gameobject_template 不存在", tablename, tmp.goLockState.goEntry, tmp.id);
                    continue;
                }
                if (!tmp.goLockState.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 search radius (datalong3 = %u)在 SCRIPT_COMMAND_GO_LOCK_STATE 中，脚本 ID:  %u.", tablename, tmp.goLockState.searchRadius, tmp.id);
                    continue;
                }
                if (// lock(0x01) and unlock(0x02) together
                    ((tmp.goLockState.lockState & 0x01) && (tmp.goLockState.lockState & 0x02)) ||
                    // non-interact (0x4) and interact (0x08) together
                    ((tmp.goLockState.lockState & 0x04) && (tmp.goLockState.lockState & 0x08)) ||
                    // no setting
                    !tmp.goLockState.lockState ||
                    //错误的 number
                    tmp.goLockState.lockState >= 0x10)
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 lock state (datalong = %u)在 SCRIPT_COMMAND_GO_LOCK_STATE 中，脚本 ID:  %u.", tablename, tmp.goLockState.lockState, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_STAND_STATE:
            {
                if (tmp.standState.stand_state >= MAX_UNIT_STAND_STATE)
                {
                    sLog.outErrorDb("数据表 `%s` 包含错误的 stand state (datalong = %u)在 SCRIPT_COMMAND_STAND_STATE 中，脚本 ID:  %u", tablename, tmp.standState.stand_state, tmp.id);
                    continue;
                }
                if (tmp.standState.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.standState.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_STAND_STATE 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.standState.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.standState.creatureEntry && !tmp.standState.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong2 = %u在 SCRIPT_COMMAND_STAND_STATE 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong3 = %u).", tablename, tmp.standState.creatureEntry, tmp.id, tmp.standState.searchRadius);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_MODIFY_NPC_FLAGS:
            {
                if (tmp.npcFlag.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.npcFlag.creatureEntry))
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong3 = %u在 SCRIPT_COMMAND_MODIFY_NPC_FLAGS 中，脚本 ID:  %u, 但 该 creature_template 不存在", tablename, tmp.run.creatureEntry, tmp.id);
                    continue;
                }
                if (tmp.npcFlag.creatureEntry && !tmp.npcFlag.searchRadius)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 datalong3 = %u在 SCRIPT_COMMAND_MODIFY_NPC_FLAGS 中，脚本 ID:  %u, 但 search radius 值太小了 (datalong4 = %u).", tablename, tmp.run.creatureEntry, tmp.id, tmp.run.searchRadius);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_GIVEXP:
            case SCRIPT_COMMAND_GIVEMONEY:
            case SCRIPT_COMMAND_GIVEHONOR:
            case SCRIPT_COMMAND_GIVEARENA:
            {
                if (tmp.GiveXp.Xp < 0 || tmp.GiveMoney.Money < 0 || tmp.GiveHonor.Honor < 0 || tmp.GiveArena.Arena < 0)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 SCRIPT_COMMAND_GIVE 但值不正确", tablename);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_GIVEPROFESSION:
            {
                if (tmp.GiveProfession.profession > 14 || tmp.GiveProfession.profession < 1 || tmp.GiveProfession.point < 0)
                {
                    sLog.outErrorDb("数据表 `%s` 包含 SCRIPT_COMMAND_GIVEPROFESSION 但值不正确(必须在 1 和 14 之间)", tablename);
                    continue;
                }
                break;
            }
        }

        if (scripts.find(tmp.id) == scripts.end())
        {
            ScriptMap emptyMap;
            scripts[tmp.id] = emptyMap;
        }
        scripts[tmp.id].insert(ScriptMap::value_type(tmp.delay, tmp));

        ++count;
    } while(result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> 加载了 %u script definitions", count);
}

void ScriptMgr::LoadGameObjectScripts()
{
    LoadScripts(sGameObjectScripts, "gameobject_scripts");

    // check ids
    for(ScriptMapMap::const_iterator itr = sGameObjectScripts.begin(); itr != sGameObjectScripts.end(); ++itr)
    {
        if (!sObjectMgr.GetGOData(itr->first))
            sLog.outErrorDb("数据表 `gameobject_scripts` 包含 not existing gameobject (GUID: %u) as script id", itr->first);
    }
}

void ScriptMgr::LoadQuestEndScripts()
{
    LoadScripts(sQuestEndScripts, "quest_end_scripts");

    // check ids
    for(ScriptMapMap::const_iterator itr = sQuestEndScripts.begin(); itr != sQuestEndScripts.end(); ++itr)
    {
        if (!sObjectMgr.GetQuestTemplate(itr->first))
            sLog.outErrorDb("数据表 `quest_end_scripts` 包含 not existing quest (Id: %u) as script id", itr->first);
    }
}

void ScriptMgr::LoadQuestStartScripts()
{
    LoadScripts(sQuestStartScripts, "quest_start_scripts");

    // check ids
    for(ScriptMapMap::const_iterator itr = sQuestStartScripts.begin(); itr != sQuestStartScripts.end(); ++itr)
    {
        if (!sObjectMgr.GetQuestTemplate(itr->first))
            sLog.outErrorDb("数据表 `quest_start_scripts` 包含 not existing quest (Id: %u) as script id", itr->first);
    }
}

void ScriptMgr::LoadSpellScripts()
{
    LoadScripts(sSpellScripts, "spell_scripts");

    // check ids
    for(ScriptMapMap::const_iterator itr = sSpellScripts.begin(); itr != sSpellScripts.end(); ++itr)
    {
        SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);

        if (!spellInfo)
        {
            sLog.outErrorDb("数据表 `spell_scripts` 包含 not existing spell (Id: %u) as script id", itr->first);
            continue;
        }

        //check for correct spellEffect
        bool found = false;
        for(int i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            // skip empty effects
            if (!spellInfo->Effect[i])
                continue;

            if (spellInfo->Effect[i] == SPELL_EFFECT_SCRIPT_EFFECT)
            {
                found =  true;
                break;
            }
        }

        if (!found)
            sLog.outErrorDb("数据表 `spell_scripts` 包含 unsupported spell (Id: %u) without SPELL_EFFECT_SCRIPT_EFFECT (%u) spell effect", itr->first, SPELL_EFFECT_SCRIPT_EFFECT);
    }
}

void ScriptMgr::LoadEventScripts()
{
    LoadScripts(sEventScripts, "event_scripts");

    std::set<uint32> evt_scripts;

    // Load all possible script entries from gameobjects
    for(uint32 i = 1; i < sGOStorage.MaxEntry; ++i)
    {
        if (GameObjectInfo const* goInfo = sGOStorage.LookupEntry<GameObjectInfo>(i))
        {
            if (uint32 eventId = goInfo->GetEventScriptId())
                evt_scripts.insert(eventId);

            if (goInfo->type == GAMEOBJECT_TYPE_CAPTURE_POINT)
            {
                evt_scripts.insert(goInfo->capturePoint.neutralEventID1);
                evt_scripts.insert(goInfo->capturePoint.neutralEventID2);
                evt_scripts.insert(goInfo->capturePoint.contestedEventID1);
                evt_scripts.insert(goInfo->capturePoint.contestedEventID2);
                evt_scripts.insert(goInfo->capturePoint.progressEventID1);
                evt_scripts.insert(goInfo->capturePoint.progressEventID2);
                evt_scripts.insert(goInfo->capturePoint.winEventID1);
                evt_scripts.insert(goInfo->capturePoint.winEventID2);
            }
        }
    }

    // Load all possible script entries from spells
    for(uint32 i = 1; i < sSpellStore.GetNumRows(); ++i)
    {
        SpellEntry const* spell = sSpellStore.LookupEntry(i);
        if (spell)
        {
            for(int j = 0; j < MAX_EFFECT_INDEX; ++j)
            {
                if (spell->Effect[j] == SPELL_EFFECT_SEND_EVENT)
                {
                    if (spell->EffectMiscValue[j])
                        evt_scripts.insert(spell->EffectMiscValue[j]);
                }
            }
        }
    }

    for(size_t path_idx = 0; path_idx < sTaxiPathNodesByPath.size(); ++path_idx)
    {
        for(size_t node_idx = 0; node_idx < sTaxiPathNodesByPath[path_idx].size(); ++node_idx)
        {
            TaxiPathNodeEntry const& node = sTaxiPathNodesByPath[path_idx][node_idx];

            if (node.arrivalEventID)
                evt_scripts.insert(node.arrivalEventID);

            if (node.departureEventID)
                evt_scripts.insert(node.departureEventID);
        }
    }

    // Then check if all scripts are在 above list of possible script entries
    for(ScriptMapMap::const_iterator itr = sEventScripts.begin(); itr != sEventScripts.end(); ++itr)
    {
        std::set<uint32>::const_iterator itr2 = evt_scripts.find(itr->first);
        if (itr2 == evt_scripts.end())
            sLog.outErrorDb("数据表 `event_scripts` 包含 script (Id: %u) not referring to any gameobject_template type 10 data2 field, type 3 data6 field, type 13 data 2 field, type 29 or any spell effect %u or path taxi node data",
                itr->first, SPELL_EFFECT_SEND_EVENT);
    }
}

void ScriptMgr::LoadGossipScripts()
{
    LoadScripts(sGossipScripts, "gossip_scripts");

    // checks are done在 LoadGossipMenuItems and LoadGossipMenu
}

void ScriptMgr::LoadCreatureMovementScripts()
{
    LoadScripts(sCreatureMovementScripts, "creature_movement_scripts");

    // checks are done在 WaypointManager::Load
}

void ScriptMgr::LoadDbScriptStrings()
{
    sObjectMgr.LoadMangosStrings(WorldDatabase, "db_script_string", MIN_DB_SCRIPT_STRING_ID, MAX_DB_SCRIPT_STRING_ID);

    std::set<int32> ids;

    for(int32 i = MIN_DB_SCRIPT_STRING_ID; i < MAX_DB_SCRIPT_STRING_ID; ++i)
        if (sObjectMgr.GetMangosStringLocale(i))
            ids.insert(i);

    CheckScriptTexts(sQuestEndScripts, ids);
    CheckScriptTexts(sQuestStartScripts, ids);
    CheckScriptTexts(sSpellScripts, ids);
    CheckScriptTexts(sGameObjectScripts, ids);
    CheckScriptTexts(sEventScripts, ids);
    CheckScriptTexts(sGossipScripts, ids);
    CheckScriptTexts(sCreatureMovementScripts, ids);

    sWaypointMgr.CheckTextsExistance(ids);

    for(std::set<int32>::const_iterator itr = ids.begin(); itr != ids.end(); ++itr)
        sLog.outErrorDb("数据表 `db_script_string` 包含 unused string id %u", *itr);
}

void ScriptMgr::CheckScriptTexts(ScriptMapMap const& scripts, std::set<int32>& ids)
{
    for(ScriptMapMap::const_iterator itrMM = scripts.begin(); itrMM != scripts.end(); ++itrMM)
    {
        for(ScriptMap::const_iterator itrM = itrMM->second.begin(); itrM != itrMM->second.end(); ++itrM)
        {
            if (itrM->second.command == SCRIPT_COMMAND_TALK)
            {
                for(int i = 0; i < MAX_TEXT_ID; ++i)
                {
                    if (itrM->second.talk.textId[i] && !sObjectMgr.GetMangosStringLocale(itrM->second.talk.textId[i]))
                        sLog.outErrorDb( "Table `db_script_string` is missing string id %u, used在 database script id %u.", itrM->second.talk.textId[i], itrMM->first);

                    if (ids.find(itrM->second.talk.textId[i]) != ids.end())
                        ids.erase(itrM->second.talk.textId[i]);
                }
            }
        }
    }
}

void ScriptMgr::LoadAreaTriggerScripts()
{
    m_AreaTriggerScripts.clear();                           // need for reload case
    QueryResult *result = WorldDatabase.Query("SELECT entry, ScriptName FROM scripted_areatrigger");

    uint32 count = 0;

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> 加载了 %u 条记录", count);
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        ++count;
        bar.step();

        Field *fields = result->Fetch();

        uint32 triggerId       = fields[0].GetUInt32();
        const char *scriptName = fields[1].GetString();

        if (!sAreaTriggerStore.LookupEntry(triggerId))
        {
            sLog.outErrorDb("数据表 `scripted_areatrigger` 包含 area trigger (ID: %u) not listed在 `AreaTrigger.dbc`.", triggerId);
            continue;
        }

        m_AreaTriggerScripts[triggerId] = GetScriptId(scriptName);
    } while(result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> 加载了 %u 条记录", count);
}

void ScriptMgr::LoadEventIdScripts()
{
    m_EventIdScripts.clear();                           // need for reload case
    QueryResult *result = WorldDatabase.Query("SELECT id, ScriptName FROM scripted_event_id");

    uint32 count = 0;

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> 加载了 %u 条记录", count);
        return;
    }

    BarGoLink bar(result->GetRowCount());

    // TODO: remove duplicate code below, same way to collect event id's used在 LoadEventScripts()
    std::set<uint32> evt_scripts;

    // Load all possible event entries from gameobjects
    for (uint32 i = 1; i < sGOStorage.MaxEntry; ++i)
    {
        if (GameObjectInfo const* goInfo = sGOStorage.LookupEntry<GameObjectInfo>(i))
        {
            if (uint32 eventId = goInfo->GetEventScriptId())
                evt_scripts.insert(eventId);

            if (goInfo->type == GAMEOBJECT_TYPE_CAPTURE_POINT)
            {
                evt_scripts.insert(goInfo->capturePoint.neutralEventID1);
                evt_scripts.insert(goInfo->capturePoint.neutralEventID2);
                evt_scripts.insert(goInfo->capturePoint.contestedEventID1);
                evt_scripts.insert(goInfo->capturePoint.contestedEventID2);
                evt_scripts.insert(goInfo->capturePoint.progressEventID1);
                evt_scripts.insert(goInfo->capturePoint.progressEventID2);
                evt_scripts.insert(goInfo->capturePoint.winEventID1);
                evt_scripts.insert(goInfo->capturePoint.winEventID2);
            }
        }
    }

    // Load all possible event entries from spells
    for(uint32 i = 1; i < sSpellStore.GetNumRows(); ++i)
    {
        SpellEntry const* spell = sSpellStore.LookupEntry(i);
        if (spell)
        {
            for(int j = 0; j < MAX_EFFECT_INDEX; ++j)
            {
                if (spell->Effect[j] == SPELL_EFFECT_SEND_EVENT)
                {
                    if (spell->EffectMiscValue[j])
                        evt_scripts.insert(spell->EffectMiscValue[j]);
                }
            }
        }
    }

    // Load all possible event entries from taxi path nodes
    for(size_t path_idx = 0; path_idx < sTaxiPathNodesByPath.size(); ++path_idx)
    {
        for(size_t node_idx = 0; node_idx < sTaxiPathNodesByPath[path_idx].size(); ++node_idx)
        {
            TaxiPathNodeEntry const& node = sTaxiPathNodesByPath[path_idx][node_idx];

            if (node.arrivalEventID)
                evt_scripts.insert(node.arrivalEventID);

            if (node.departureEventID)
                evt_scripts.insert(node.departureEventID);
        }
    }

    do
    {
        ++count;
        bar.step();

        Field *fields = result->Fetch();

        uint32 eventId          = fields[0].GetUInt32();
        const char *scriptName  = fields[1].GetString();

        std::set<uint32>::const_iterator itr = evt_scripts.find(eventId);
        if (itr == evt_scripts.end())
            sLog.outErrorDb("数据表 `scripted_event_id` 包含 id %u not referring to any gameobject_template type 10 data2 field, type 3 data6 field, type 13 data 2 field, type 29 or any spell effect %u or path taxi node data",
                eventId, SPELL_EFFECT_SEND_EVENT);

        m_EventIdScripts[eventId] = GetScriptId(scriptName);
    } while(result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> 加载了 %u 条记录", count);
}

void ScriptMgr::LoadScriptNames()
{
    m_scriptNames.push_back("");
    QueryResult *result = WorldDatabase.Query(
      "SELECT DISTINCT(ScriptName) FROM creature_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM gameobject_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM item_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM scripted_areatrigger WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM scripted_event_id WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM instance_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM world_template WHERE ScriptName <> ''");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString();
        sLog.outErrorDb(">> 加载了 0 条记录");
        return;
    }

    BarGoLink bar(result->GetRowCount());
    uint32 count = 0;

    do
    {
        bar.step();
        m_scriptNames.push_back((*result)[0].GetString());
        ++count;
    } while (result->NextRow());
    delete result;

    std::sort(m_scriptNames.begin(), m_scriptNames.end());
    sLog.outString();
    sLog.outString(">> 加载了 %d 个脚本", count);
}

uint32 ScriptMgr::GetScriptId(const char *name) const
{
    // use binary search to find the script name在 the sorted vector
    // assume "" is the first element
    if (!name)
        return 0;

    ScriptNameMap::const_iterator itr =
        std::lower_bound(m_scriptNames.begin(), m_scriptNames.end(), name);

    if (itr == m_scriptNames.end() || *itr != name)
        return 0;

    return uint32(itr - m_scriptNames.begin());
}

uint32 ScriptMgr::GetAreaTriggerScriptId(uint32 triggerId) const
{
    AreaTriggerScriptMap::const_iterator itr = m_AreaTriggerScripts.find(triggerId);
    if (itr != m_AreaTriggerScripts.end())
        return itr->second;

    return 0;
}

uint32 ScriptMgr::GetEventIdScriptId(uint32 eventId) const
{
    EventIdScriptMap::const_iterator itr = m_EventIdScripts.find(eventId);
    if (itr != m_EventIdScripts.end())
        return itr->second;

    return 0;
}

char const* ScriptMgr::GetScriptLibraryVersion() const
{
    if (!m_pGetScriptLibraryVersion)
        return "";

    return m_pGetScriptLibraryVersion();
}

CreatureAI* ScriptMgr::GetCreatureAI(Creature* pCreature)
{
    if (!m_pGetCreatureAI)
        return NULL;

    return m_pGetCreatureAI(pCreature);
}

InstanceData* ScriptMgr::CreateInstanceData(Map* pMap)
{
    if (!m_pCreateInstanceData)
        return NULL;

    return m_pCreateInstanceData(pMap);
}

bool ScriptMgr::OnGossipHello(Player* pPlayer, Creature* pCreature)
{
    return m_pOnGossipHello != NULL && m_pOnGossipHello(pPlayer, pCreature);
}

bool ScriptMgr::OnGossipHello(Player* pPlayer, GameObject* pGameObject)
{
    return m_pOnGOGossipHello != NULL && m_pOnGOGossipHello(pPlayer, pGameObject);
}

bool ScriptMgr::OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action, const char* code)
{
    if (code)
        return m_pOnGossipSelectWithCode != NULL && m_pOnGossipSelectWithCode(pPlayer, pCreature, sender, action, code);
    else
        return m_pOnGossipSelect != NULL && m_pOnGossipSelect(pPlayer, pCreature, sender, action);
}

bool ScriptMgr::OnGossipSelect(Player* pPlayer, GameObject* pGameObject, uint32 sender, uint32 action, const char* code)
{
    if (code)
        return m_pOnGOGossipSelectWithCode != NULL && m_pOnGOGossipSelectWithCode(pPlayer, pGameObject, sender, action, code);
    else
        return m_pOnGOGossipSelect != NULL && m_pOnGOGossipSelect(pPlayer, pGameObject, sender, action);
}

bool ScriptMgr::OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    return m_pOnQuestAccept != NULL && m_pOnQuestAccept(pPlayer, pCreature, pQuest);
}

bool ScriptMgr::OnQuestAccept(Player* pPlayer, GameObject* pGameObject, Quest const* pQuest)
{
    return m_pOnGOQuestAccept != NULL && m_pOnGOQuestAccept(pPlayer, pGameObject, pQuest);
}

bool ScriptMgr::OnQuestAccept(Player* pPlayer, Item* pItem, Quest const* pQuest)
{
    return m_pOnItemQuestAccept != NULL && m_pOnItemQuestAccept(pPlayer, pItem, pQuest);
}

bool ScriptMgr::OnQuestRewarded(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    return m_pOnQuestRewarded != NULL && m_pOnQuestRewarded(pPlayer, pCreature, pQuest);
}

bool ScriptMgr::OnQuestRewarded(Player* pPlayer, GameObject* pGameObject, Quest const* pQuest)
{
    return m_pOnGOQuestRewarded != NULL && m_pOnGOQuestRewarded(pPlayer, pGameObject, pQuest);
}

uint32 ScriptMgr::GetDialogStatus(Player* pPlayer, Creature* pCreature)
{
    if (!m_pGetNPCDialogStatus)
        return 100;

    return m_pGetNPCDialogStatus(pPlayer, pCreature);
}

uint32 ScriptMgr::GetDialogStatus(Player* pPlayer, GameObject* pGameObject)
{
    if (!m_pGetGODialogStatus)
        return 100;

    return m_pGetGODialogStatus(pPlayer, pGameObject);
}

bool ScriptMgr::OnGameObjectUse(Player* pPlayer, GameObject* pGameObject)
{
    return m_pOnGOUse != NULL && m_pOnGOUse(pPlayer, pGameObject);
}

bool ScriptMgr::OnItemUse(Player* pPlayer, Item* pItem, SpellCastTargets const& targets)
{
    return m_pOnItemUse != NULL && m_pOnItemUse(pPlayer, pItem, targets);
}

bool ScriptMgr::OnGossipSelectItem(Player* pPlayer, Item* pItem, uint32 sender, uint32 action, SpellCastTargets const& targets)
{
    return m_pOnGossipSelectItem != NULL && m_pOnGossipSelectItem(pPlayer, pItem, sender, action, targets);
}

bool ScriptMgr::OnAreaTrigger(Player* pPlayer, AreaTriggerEntry const* atEntry)
{
    return m_pOnAreaTrigger != NULL && m_pOnAreaTrigger(pPlayer, atEntry);
}

bool ScriptMgr::OnProcessEvent(uint32 eventId, Object* pSource, Object* pTarget, bool isStart)
{
    return m_pOnProcessEvent != NULL && m_pOnProcessEvent(eventId, pSource, pTarget, isStart);
}

bool ScriptMgr::OnEffectDummy(Unit* pCaster, uint32 spellId, SpellEffectIndex effIndex, Creature* pTarget)
{
    return m_pOnEffectDummyCreature != NULL && m_pOnEffectDummyCreature(pCaster, spellId, effIndex, pTarget);
}

bool ScriptMgr::OnEffectDummy(Unit* pCaster, uint32 spellId, SpellEffectIndex effIndex, GameObject* pTarget)
{
    return m_pOnEffectDummyGO != NULL && m_pOnEffectDummyGO(pCaster, spellId, effIndex, pTarget);
}

bool ScriptMgr::OnEffectDummy(Unit* pCaster, uint32 spellId, SpellEffectIndex effIndex, Item* pTarget)
{
    return m_pOnEffectDummyItem != NULL && m_pOnEffectDummyItem(pCaster, spellId, effIndex, pTarget);
}

bool ScriptMgr::OnAuraDummy(Aura const* pAura, bool apply)
{
    return m_pOnAuraDummy != NULL && m_pOnAuraDummy(pAura, apply);
}

ScriptLoadResult ScriptMgr::LoadScriptLibrary(const char* libName)
{
    UnloadScriptLibrary();

    std::string name = libName;
    name = MANGOS_SCRIPT_PREFIX + name + MANGOS_SCRIPT_SUFFIX;

    m_hScriptLib = MANGOS_LOAD_LIBRARY(name.c_str());

    sLog.outString( ">> 加载 %s 脚本库", name.c_str());

    if (!m_hScriptLib)
        return SCRIPT_LOAD_ERR_NOT_FOUND;

#   define GET_SCRIPT_HOOK_PTR(P,N)             \
        GetScriptHookPtr((P), (N));             \
        if (!(P))                               \
        {                                       \
            sLog.outError("ScriptMgr::LoadScriptLibrary(): 函数 %s 丢失！", N); \
            /* prevent call before init */      \
            m_pOnFreeScriptLibrary = NULL;      \
            UnloadScriptLibrary();              \
            return SCRIPT_LOAD_ERR_WRONG_API;   \
        }

    // let check used mangosd revision for build library (unsafe use with different revision because changes在 inline functions, define and etc)
    char const* (MANGOS_IMPORT* pGetMangosRevStr) ();

    GET_SCRIPT_HOOK_PTR(pGetMangosRevStr,              "GetMangosRevStr");

    GET_SCRIPT_HOOK_PTR(m_pOnInitScriptLibrary,        "InitScriptLibrary");
    GET_SCRIPT_HOOK_PTR(m_pOnFreeScriptLibrary,        "FreeScriptLibrary");
    GET_SCRIPT_HOOK_PTR(m_pGetScriptLibraryVersion,    "GetScriptLibraryVersion");

    GET_SCRIPT_HOOK_PTR(m_pGetCreatureAI,              "GetCreatureAI");
    GET_SCRIPT_HOOK_PTR(m_pCreateInstanceData,         "CreateInstanceData");

    GET_SCRIPT_HOOK_PTR(m_pOnGossipHello,              "GossipHello");
    GET_SCRIPT_HOOK_PTR(m_pOnGOGossipHello,            "GOGossipHello");
    GET_SCRIPT_HOOK_PTR(m_pOnGossipSelect,             "GossipSelect");
    GET_SCRIPT_HOOK_PTR(m_pOnGOGossipSelect,           "GOGossipSelect");
    GET_SCRIPT_HOOK_PTR(m_pOnGossipSelectWithCode,     "GossipSelectWithCode");
    GET_SCRIPT_HOOK_PTR(m_pOnGOGossipSelectWithCode,   "GOGossipSelectWithCode");
    GET_SCRIPT_HOOK_PTR(m_pOnQuestAccept,              "QuestAccept");
    GET_SCRIPT_HOOK_PTR(m_pOnGOQuestAccept,            "GOQuestAccept");
    GET_SCRIPT_HOOK_PTR(m_pOnItemQuestAccept,          "ItemQuestAccept");
    GET_SCRIPT_HOOK_PTR(m_pOnQuestRewarded,            "QuestRewarded");
    GET_SCRIPT_HOOK_PTR(m_pOnGOQuestRewarded,          "GOQuestRewarded");
    GET_SCRIPT_HOOK_PTR(m_pGetNPCDialogStatus,         "GetNPCDialogStatus");
    GET_SCRIPT_HOOK_PTR(m_pGetGODialogStatus,          "GetGODialogStatus");
    GET_SCRIPT_HOOK_PTR(m_pOnGOUse,                    "GOUse");
    GET_SCRIPT_HOOK_PTR(m_pOnItemUse,                  "ItemUse");
    GET_SCRIPT_HOOK_PTR(m_pOnGossipSelectItem,         "GossipSelectItem");
    GET_SCRIPT_HOOK_PTR(m_pOnAreaTrigger,              "AreaTrigger");
    GET_SCRIPT_HOOK_PTR(m_pOnProcessEvent,             "ProcessEvent");
    GET_SCRIPT_HOOK_PTR(m_pOnEffectDummyCreature,      "EffectDummyCreature");
    GET_SCRIPT_HOOK_PTR(m_pOnEffectDummyGO,            "EffectDummyGameObject");
    GET_SCRIPT_HOOK_PTR(m_pOnEffectDummyItem,          "EffectDummyItem");
    GET_SCRIPT_HOOK_PTR(m_pOnAuraDummy,                "AuraDummy");

#   undef GET_SCRIPT_HOOK_PTR

    if (strcmp(pGetMangosRevStr(), REVISION_NR) != 0)
    {
        m_pOnFreeScriptLibrary = NULL;                      // prevent call before init
        UnloadScriptLibrary();
        return SCRIPT_LOAD_ERR_OUTDATED;
    }

    m_pOnInitScriptLibrary();
    return SCRIPT_LOAD_OK;
}

void ScriptMgr::UnloadScriptLibrary()
{
    if (!m_hScriptLib)
        return;

    if (m_pOnFreeScriptLibrary)
        m_pOnFreeScriptLibrary();

    MANGOS_CLOSE_LIBRARY(m_hScriptLib);
    m_hScriptLib = NULL;

    m_pOnInitScriptLibrary      = NULL;
    m_pOnFreeScriptLibrary      = NULL;
    m_pGetScriptLibraryVersion  = NULL;

    m_pGetCreatureAI            = NULL;
    m_pCreateInstanceData       = NULL;

    m_pOnGossipHello            = NULL;
    m_pOnGOGossipHello          = NULL;
    m_pOnGossipSelect           = NULL;
    m_pOnGOGossipSelect         = NULL;
    m_pOnGossipSelectWithCode   = NULL;
    m_pOnGOGossipSelectWithCode = NULL;
    m_pOnQuestAccept            = NULL;
    m_pOnGOQuestAccept          = NULL;
    m_pOnItemQuestAccept        = NULL;
    m_pOnQuestRewarded          = NULL;
    m_pOnGOQuestRewarded        = NULL;
    m_pGetNPCDialogStatus       = NULL;
    m_pGetGODialogStatus        = NULL;
    m_pOnGOUse                  = NULL;
    m_pOnItemUse                = NULL;
    m_pOnGossipSelectItem       = NULL;
    m_pOnAreaTrigger            = NULL;
    m_pOnProcessEvent           = NULL;
    m_pOnEffectDummyCreature    = NULL;
    m_pOnEffectDummyGO          = NULL;
    m_pOnEffectDummyItem        = NULL;
    m_pOnAuraDummy              = NULL;
}

uint32 GetAreaTriggerScriptId(uint32 triggerId)
{
    return sScriptMgr.GetAreaTriggerScriptId(triggerId);
}

uint32 GetEventIdScriptId(uint32 eventId)
{
    return sScriptMgr.GetEventIdScriptId(eventId);
}

uint32 GetScriptId(const char *name)
{
    return sScriptMgr.GetScriptId(name);
}

char const* GetScriptName(uint32 id)
{
    return sScriptMgr.GetScriptName(id);
}

uint32 GetScriptIdsCount()
{
    return sScriptMgr.GetScriptIdsCount();
}


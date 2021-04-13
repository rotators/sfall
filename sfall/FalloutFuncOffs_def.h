// X-Macro for function offsets

FUNC(AddHotLines_,                    0x4998C0)
FUNC(Check4Keys_,                     0x43F73C)
FUNC(Create_AudioDecoder_,            0x4D50A8)
FUNC(DOSCmdLineDestroy_,              0x4E3D3C)
FUNC(DrawCard_,                       0x43AAEC)
FUNC(DrawFolder_,                     0x43410C)
FUNC(DrawInfoWin_,                    0x4365AC)
FUNC(EndLoad_,                        0x47F4C8)
FUNC(EndPipboy_,                      0x497828)
FUNC(FMtext_char_width_,              0x4421DC)
FUNC(FMtext_to_buf_,                  0x4422B4)
FUNC(FMtext_width_,                   0x442188)
FUNC(GNW95_lost_focus_,               0x4C9EEC)
FUNC(GNW95_process_message_,          0x4C9CF0)
FUNC(GNW_button_refresh_,             0x4D9A58)
FUNC(GNW_do_bk_process_,              0x4C8D1C)
FUNC(GNW_find_,                       0x4D7888)
FUNC(GNW_win_refresh_,                0x4D6FD8)
FUNC(GetSlotList_,                    0x47E5D0)
FUNC(ListDPerks_,                     0x43D0BC)
FUNC(ListDrvdStats_,                  0x43527C)
FUNC(ListHoloDiskTitles_,             0x498C40)
FUNC(ListSkills_,                     0x436154)
FUNC(ListTraits_,                     0x43B8A8)
FUNC(LoadGame_,                       0x47C640)
FUNC(LoadSlot_,                       0x47DC68)
FUNC(MapDirErase_,                    0x480040)
FUNC(NixHotLines_,                    0x4999C0)
FUNC(OptionWindow_,                   0x437C08)
FUNC(PipStatus_,                      0x497BD8)
FUNC(PrintBasicStat_,                 0x434B38)
FUNC(PrintLevelWin_,                  0x434920)
FUNC(RestorePlayer_,                  0x43A8BC)
FUNC(SaveGame_,                       0x47B88C)
FUNC(SavePlayer_,                     0x43A7DC)
FUNC(SexWindow_,                      0x437664)
FUNC(_word_wrap_,                     0x4BC6F0)
FUNC(action_get_an_object_,           0x412134)
FUNC(action_loot_container_,          0x4123E8)
FUNC(action_use_an_item_on_object_,   0x411F2C)
FUNC(add_bar_box_,                    0x4616F0)
FUNC(adjust_ac_,                      0x4715F8)
FUNC(adjust_fid_,                     0x4716E8)
FUNC(ai_can_use_weapon_,              0x4298EC) // (TGameObj *aCritter<eax>, int aWeapon<edx>, int a2Or3<ebx>) returns 1 or 0
FUNC(ai_cap_,                         0x4280B4)
FUNC(ai_check_drugs_,                 0x428480)
FUNC(ai_have_ammo_,                   0x4292D4)
FUNC(ai_pick_hit_mode_,               0x429DB4)
FUNC(ai_retrieve_object_,             0x429D60)
FUNC(ai_run_away_,                    0x428868)
FUNC(ai_search_inven_armor_,          0x429A6C)
FUNC(ai_search_inven_weap_,           0x4299A0)
FUNC(ai_switch_weapons_,              0x42A5B8)
FUNC(ai_try_attack_,                  0x42A7D8)
FUNC(anim_can_use_door_,              0x415E24)
FUNC(art_alias_fid_,                  0x4199D4)
FUNC(art_alias_num_,                  0x419998)
FUNC(art_exists_,                     0x4198C8) // eax - frameID, used for critter FIDs
FUNC(art_flush_,                      0x41927C)
FUNC(art_frame_data_,                 0x419870)
FUNC(art_frame_length_,               0x4197B8)
FUNC(art_frame_width_,                0x4197A0)
FUNC(art_get_code_,                   0x419314)
FUNC(art_get_name_,                   0x419428)
FUNC(art_id_,                         0x419C88)
FUNC(art_init_,                       0x418840)
FUNC(art_lock_,                       0x4191CC)
FUNC(art_ptr_lock_,                   0x419160)
FUNC(art_ptr_lock_data_,              0x419188)
FUNC(art_ptr_unlock_,                 0x419260)
FUNC(attack_crit_success_,            0x423EB4)
FUNC(audioCloseFile_,                 0x41A50C)
FUNC(audioFileSize_,                  0x41A78C)
FUNC(audioOpen_,                      0x41A2EC)
FUNC(audioRead_,                      0x41A574)
FUNC(audioSeek_,                      0x41A5E0)
FUNC(automap_,                        0x41B8BC)
FUNC(barter_compute_value_,           0x474B2C)
FUNC(barter_inventory_,               0x4757F0)
FUNC(block_for_tocks_,                0x4C93B8)
FUNC(buf_to_buf_,                     0x4D36D4)
FUNC(caiHasWeapPrefType_,             0x42938C)
FUNC(cai_attempt_w_reload_,           0x42AECC)
FUNC(cai_retargetTileFromFriendlyFireSubFunc_,   0x42A410)
FUNC(cai_retargetTileFromFriendlyFire_,          0x42A1D4)
FUNC(can_see_,                        0x412BEC)
FUNC(check_death_,                    0x410814)
FUNC(check_for_death_,                0x424EE8)
FUNC(combat_,                         0x422D2C)
FUNC(combat_ai_,                      0x42B130)
FUNC(combat_anim_finished_,           0x425E80)
FUNC(combat_attack_,                  0x422F3C)
FUNC(combat_check_bad_shot_,          0x426614)
FUNC(combat_ctd_init_,                0x422EC4)
FUNC(combat_delete_critter_,          0x426DDC)
FUNC(combat_input_,                   0x4227F4)
FUNC(combat_should_end_,              0x422C60)
FUNC(combat_turn_,                    0x42299C)
FUNC(combat_turn_run_,                0x4227DC)
FUNC(combatai_check_retaliation_,     0x42B9D4)
FUNC(combatai_rating_,                0x42B90C)
FUNC(compute_damage_,                 0x4247B8)
FUNC(compute_spray_,                  0x423488)
FUNC(config_get_string_,              0x42BF48)
FUNC(config_get_value_,               0x42C05C)
FUNC(config_set_value_,               0x42C160)
FUNC(construct_box_bar_win_,          0x461134)
FUNC(container_exit_,                 0x476394)
FUNC(correctFidForRemovedItem_,       0x45409C) // (int critter@<eax>, int oldArmor@<edx>, int removeSlotsFlags@<ebx>)
FUNC(createWindow_,                   0x4B7F3C)
FUNC(credits_,                        0x42C860)
FUNC(credits_get_next_line_,          0x42CE6C)
FUNC(critterClearObjDrugs_,           0x42DA54)
FUNC(critterIsOverloaded_,            0x42E66C)
FUNC(critter_adjust_hits_,            0x42D1A4)
FUNC(critter_body_type_,              0x42DDC4)
FUNC(critter_can_obj_dude_rest_,      0x42E564)
FUNC(critter_compute_ap_from_distance_,          0x42E62C)
FUNC(critter_flag_check_,             0x42E6AC)
FUNC(critter_get_hits_,               0x42D18C)
FUNC(critter_get_rads_,               0x42D38C)
FUNC(critter_is_dead_,                0x42DD18)
FUNC(critter_kill_,                   0x42DA64)
FUNC(critter_kill_count_type_,        0x42D920)
FUNC(critter_name_,                   0x42D0A8)
FUNC(critter_pc_set_name_,            0x42D138)
FUNC(critter_wake_clear_,             0x42E460)
FUNC(critter_wake_up_,                0x42E424)
FUNC(datafileConvertData_,            0x42EE84)
FUNC(db_access_,                      0x4390B4)
FUNC(db_dir_entry_,                   0x4C5D68)
FUNC(db_fclose_,                      0x4C5EB4)
FUNC(db_fgetc_,                       0x4C5F24)
FUNC(db_fgets_,                       0x4C5F70)
FUNC(db_fopen_,                       0x4C5EC8)
FUNC(db_freadByteCount_,              0x4C62FC)
FUNC(db_freadByte_,                   0x4C60E0)
FUNC(db_freadIntCount_,               0x4C63BC)
FUNC(db_freadInt_,                    0x4C614C)
FUNC(db_freadShortCount_,             0x4C6330)
FUNC(db_freadShort_,                  0x4C60F4)
FUNC(db_fread_,                       0x4C5FFC)
FUNC(db_free_file_list_,              0x4C6868)
FUNC(db_fseek_,                       0x4C60C0)
FUNC(db_fwriteByteCount_,             0x4C6464)
FUNC(db_fwriteByte_,                  0x4C61AC)
FUNC(db_fwriteInt_,                   0x4C6214)
FUNC(db_get_file_list_,               0x4C6628)
FUNC(db_init_,                        0x4C5D30)
FUNC(db_read_to_buf_,                 0x4C5DD4)
FUNC(dbase_close_,                    0x4E5270)
FUNC(dbase_open_,                     0x4E4F58)
FUNC(debug_log_,                      0x4C7028)
FUNC(debug_printf_,                   0x4C6F48)
FUNC(debug_register_env_,             0x4C6D90)
FUNC(determine_to_hit_,               0x42436C)
FUNC(determine_to_hit_from_tile_,     0x424394)
FUNC(determine_to_hit_func_,          0x4243A8)
FUNC(determine_to_hit_no_range_,      0x424380)
FUNC(dialog_out_,                     0x41CF20)
FUNC(displayInWindow_,                0x4B8B10)
FUNC(display_inventory_,              0x46FDF4)
FUNC(display_print_,                  0x43186C)
FUNC(display_scroll_down_,            0x431B9C)
FUNC(display_scroll_up_,              0x431B70)
FUNC(display_stats_,                  0x471D5C)
FUNC(display_table_inventories_,      0x475334)
FUNC(display_target_inventory_,       0x47036C)
FUNC(do_optionsFunc_,                 0x48FC50)
FUNC(do_options_,                     0x48FC48)
FUNC(do_prefscreen_,                  0x490798)
FUNC(drop_into_container_,            0x476464)
FUNC(dude_stand_,                     0x418378)
FUNC(dude_standup_,                   0x418574)
FUNC(editor_design_,                  0x431DF8)
FUNC(elapsed_time_,                   0x4C93E0)
FUNC(elevator_end_,                   0x43F6D0)
FUNC(elevator_start_,                 0x43F324)
FUNC(endgame_slideshow_,              0x43F788)
FUNC(exec_script_proc_,               0x4A4810) // unsigned int aScriptID<eax>, int aProcId<edx>
FUNC(executeProcedure_,               0x46DD2C) // <eax> - programPtr, <edx> - procNumber
FUNC(exit_inventory_,                 0x46FBD8)
FUNC(fadeSystemPalette_,              0x4C7320)
FUNC(findCurrentProc_,                0x467160)
FUNC(findVar_,                        0x4410AC)
FUNC(folder_print_line_,              0x43E3D8)
FUNC(fprintf_,                        0x4F0D56)
FUNC(frame_ptr_,                      0x419880)
FUNC(game_exit_,                      0x442C34)
FUNC(game_get_global_var_,            0x443C68)
FUNC(game_help_,                      0x443F74)
FUNC(game_reset_,                     0x442B84)
FUNC(game_set_global_var_,            0x443C98)
FUNC(game_time_,                      0x4A3330)
FUNC(game_time_date_,                 0x4A3338)
FUNC(gdDestroyHeadWindow_,            0x447294)
FUNC(gdProcess_,                      0x4465C0)
FUNC(gdReviewExit_,                   0x445C18)
FUNC(gdReviewInit_,                   0x445938)
FUNC(gdialogActive_,                  0x444D2C)
FUNC(gdialogDisplayMsg_,              0x445448)
FUNC(gdialogFreeSpeech_,              0x4450C4)
FUNC(gdialog_barter_cleanup_tables_,  0x448660)
FUNC(gdialog_barter_pressed_,         0x44A52C)
FUNC(gdialog_window_create_,          0x44A62C)
FUNC(gdialog_window_destroy_,         0x44A9D8)
FUNC(get_input_,                      0x4C8B78)
FUNC(get_input_str2_,                 0x47F084)
FUNC(get_time_,                       0x4C9370)
FUNC(getmsg_,                         0x48504C) // eax - msg file addr, ebx - message ID, edx - int[4]  - loads string from MSG file preloaded in memory
//FUNC(gmouse_3d_get_mode_,           0x44CB6C)
FUNC(gmouse_3d_set_mode_,             0x44CA18)
FUNC(gmouse_is_scrolling_,            0x44B54C)
FUNC(gmouse_set_cursor_,              0x44C840)
FUNC(gmovieIsPlaying_,                0x44EB14)
FUNC(gmovie_play_,                    0x44E690)
FUNC(gsnd_build_weapon_sfx_name_,     0x451760)
FUNC(gsound_background_pause_,        0x450B50)
FUNC(gsound_background_restart_last_, 0x450B0C)
FUNC(gsound_background_stop_,         0x450AB4)
FUNC(gsound_background_unpause_,      0x450B64)
FUNC(gsound_background_volume_get_set_,          0x450620)
FUNC(gsound_play_sfx_file_,           0x4519A8)
FUNC(gsound_red_butt_press_,          0x451970)
FUNC(gsound_red_butt_release_,        0x451978)
FUNC(gsound_speech_length_get_,       0x450C94)
FUNC(gsound_speech_play_,             0x450CA0)
FUNC(handle_inventory_,               0x46E7B0)
FUNC(inc_game_time_,                  0x4A34CC)
FUNC(inc_stat_,                       0x4AF5D4)
FUNC(insert_withdrawal_,              0x47A290)
FUNC(interpretAddString_,             0x467A80) // edx = ptr to string, eax = script
FUNC(interpretError_,                 0x4671F0)
FUNC(interpretFindProcedure_,         0x46DCD0) // get proc number (different for each script) by name: *<eax> - scriptPtr, char* <edx> - proc name
FUNC(interpretFreeProgram_,           0x467614) // <eax> - program ptr, frees it from memory and from scripting engine
FUNC(interpretGetString_,             0x4678E0) // eax = script ptr, edx = var type, ebx = var
FUNC(interpretPopLong_,               0x467500)
FUNC(interpretPopShort_,              0x4674F0)
FUNC(interpretPushLong_,              0x4674DC)
FUNC(interpretPushShort_,             0x46748C)
FUNC(interpret_,                      0x46CCA4)
FUNC(intface_disable_,                0x45EAFC)
FUNC(intface_enable_,                 0x45EA64)
FUNC(intface_get_attack_,             0x45EF6C)
FUNC(intface_hide_,                   0x45E9E0)
FUNC(intface_is_hidden_,              0x45EA5C)
FUNC(intface_is_item_right_hand_,     0x45F7FC)
FUNC(intface_item_reload_,            0x460B20)
FUNC(intface_redraw_,                 0x45EB98)
FUNC(intface_show_,                   0x45EA10)
FUNC(intface_toggle_item_state_,      0x45F4E0)
FUNC(intface_toggle_items_,           0x45F404)
FUNC(intface_update_ac_,              0x45EDA8)
FUNC(intface_update_hit_points_,      0x45EBD8)
FUNC(intface_update_items_,           0x45EFEC)
FUNC(intface_update_move_points_,     0x45EE0C)
FUNC(intface_use_item_,               0x45F5EC)
FUNC(invenUnwieldFunc_,               0x472A64) // (int critter@<eax>, int slot@<edx>, int a3@<ebx>) - int result (-1 on error, 0 on success)
FUNC(invenWieldFunc_,                 0x472768) // (int who@<eax>, int item@<edx>, int a3@<ecx>, int slot@<ebx>) - int result (-1 on error, 0 on success)
FUNC(inven_display_msg_,              0x472D24)
FUNC(inven_find_id_,                  0x4726EC)
FUNC(inven_find_type_,                0x472698)
FUNC(inven_left_hand_,                0x471BBC)
FUNC(inven_pid_is_carried_ptr_,       0x471CA0)
FUNC(inven_right_hand_,               0x471B70)
FUNC(inven_set_mouse_,                0x470BCC)
FUNC(inven_unwield_,                  0x472A54)
FUNC(inven_wield_,                    0x472758)
FUNC(inven_worn_,                     0x471C08)
FUNC(isPartyMember_,                  0x494FC4) // (<eax> - object) - bool result
FUNC(is_pc_sneak_working_,            0x42E3F4)
FUNC(is_within_perception_,           0x42BA04)
FUNC(item_add_force_,                 0x4772B8)
FUNC(item_add_mult_,                  0x477158)
FUNC(item_c_curr_size_,               0x479A20)
FUNC(item_c_max_size_,                0x479A00)
FUNC(item_caps_total_,                0x47A6A8)
FUNC(item_count_,                     0x47808C)
FUNC(item_d_check_addict_,            0x47A640)
FUNC(item_d_take_drug_,               0x479F60)
FUNC(item_drop_all_,                  0x477804)
FUNC(item_get_type_,                  0x477AFC)
FUNC(item_hit_with_,                  0x477FF8)
FUNC(item_m_cell_pid_,                0x479454)
FUNC(item_m_dec_charges_,             0x4795A4)
FUNC(item_m_turn_off_,                0x479898)
FUNC(item_move_all_,                  0x4776AC)
FUNC(item_move_all_hidden_,           0x4776E0)
FUNC(item_move_force_,                0x4776A4)
FUNC(item_mp_cost_,                   0x478040)
FUNC(item_remove_mult_,               0x477490)
FUNC(item_size_,                      0x477B68)
FUNC(item_total_cost_,                0x477DAC)
FUNC(item_total_weight_,              0x477E98)
FUNC(item_w_anim_code_,               0x478DA8)
FUNC(item_w_anim_weap_,               0x47860C)
FUNC(item_w_can_reload_,              0x478874)
FUNC(item_w_compute_ammo_cost_,       0x4790AC) // signed int aWeapon<eax>, int *aRoundsSpent<edx>
FUNC(item_w_curr_ammo_,               0x4786A0)
FUNC(item_w_dam_div_,                 0x479294)
FUNC(item_w_dam_mult_,                0x479230)
FUNC(item_w_damage_,                  0x478448)
FUNC(item_w_damage_type_,             0x478570)
FUNC(item_w_dr_adjust_,               0x4791E0)
FUNC(item_w_max_ammo_,                0x478674)
FUNC(item_w_mp_cost_,                 0x478B24)
FUNC(item_w_perk_,                    0x478D58)
FUNC(item_w_primary_mp_cost_,         0x47905C)
FUNC(item_w_range_,                   0x478A1C)
FUNC(item_w_reload_,                  0x478918)
FUNC(item_w_rounds_,                  0x478D80)
FUNC(item_w_secondary_mp_cost_,       0x479084)
FUNC(item_w_subtype_,                 0x478280)
FUNC(item_w_try_reload_,              0x478768)
FUNC(item_w_unload_,                  0x478F80)
FUNC(item_weight_,                    0x477B88)
FUNC(kb_clear_,                       0x4CBDA8)
FUNC(light_get_tile_,                 0x47A980) // aElev<eax>, aTilenum<edx>
FUNC(loadColorTable_,                 0x4C78E4)
FUNC(loadPCX_,                        0x496494)
FUNC(loadProgram_,                    0x4A3B74) // loads script from scripts/ folder by file name and returns pointer to it: char* <eax> - file name (w/o extension)
FUNC(load_frame_,                     0x419EC0)
FUNC(loot_container_,                 0x473904)
FUNC(main_game_loop_,                 0x480E48)
FUNC(main_init_system_,               0x480CC0)
FUNC(main_load_new_,                  0x480D4C)
FUNC(main_menu_create_,               0x481650)
FUNC(main_menu_hide_,                 0x481A00)
FUNC(main_menu_loop_,                 0x481AEC)
// (int aObjFrom<eax>, int aTileFrom<edx>, char* aPathPtr<ecx>, int aTileTo<ebx>, int a5, int (__fastcall *a6)(_DWORD, _DWORD))
// - path is saved in ecx as a sequence of tile directions (0..5) to move on each step,
// - returns path length
FUNC(make_path_func_,                 0x415EFC)
FUNC(make_straight_path_,             0x4163AC)
FUNC(make_straight_path_func_,        0x4163C8) // (TGameObj *aObj<eax>, int aTileFrom<edx>, int a3<ecx>, signed int aTileTo<ebx>, TGameObj **aObjResult, int a5, int (*a6)(void))
FUNC(map_disable_bk_processes_,       0x482104)
FUNC(map_enable_bk_processes_,        0x4820C0)
FUNC(map_exit_,                       0x482084)
FUNC(map_get_short_name_,             0x48261C)
FUNC(map_load_idx_,                   0x482B34)
FUNC(mem_free_,                       0x4C5C24)
FUNC(mem_malloc_,                     0x4C5AD0)
FUNC(mem_realloc_,                    0x4C5B50)
FUNC(message_add_,                    0x484D68)
FUNC(message_exit_,                   0x484964)
FUNC(message_filter_,                 0x485078)
FUNC(message_find_,                   0x484D10)
FUNC(message_init_,                   0x48494C)
FUNC(message_load_,                   0x484AA4)
FUNC(message_make_path_,              0x484CB8)
FUNC(message_search_,                 0x484C30)
FUNC(mouse_click_in_,                 0x4CA934)
FUNC(mouse_get_position_,             0x4CA9DC)
FUNC(mouse_get_rect_,                 0x4CA9A0)
FUNC(mouse_hide_,                     0x4CA534)
FUNC(mouse_in_,                       0x4CA8C8)
FUNC(mouse_show_,                     0x4CA34C)
FUNC(move_inventory_,                 0x474708)
FUNC(movieRun_,                       0x487AC8)
FUNC(movieStop_,                      0x487150)
FUNC(movieUpdate_,                    0x487BEC)
FUNC(my_free_,                        0x4C5C2C)
FUNC(new_obj_id_,                     0x4A386C)
FUNC(nrealloc_,                       0x4F1669)
FUNC(obj_ai_blocking_at_,             0x48BA20)
FUNC(obj_blocking_at_,                0x48B848) // <eax>(int aExcludeObject<eax> /* can be 0 */, signed int aTile<edx>, int aElevation<ebx>)
FUNC(obj_bound_,                      0x48B66C)
FUNC(obj_change_fid_,                 0x48AA3C)
FUNC(obj_connect_,                    0x489EC4)
FUNC(obj_destroy_,                    0x49B9A0)
FUNC(obj_dist_,                       0x48BBD4)
FUNC(obj_dist_with_tile_,             0x48BC08)
FUNC(obj_drop_,                       0x49B8B0)
FUNC(obj_erase_object_,               0x48B0FC)
FUNC(obj_examine_,                    0x49AD78)
FUNC(obj_find_first_,                 0x48B3A8)
FUNC(obj_find_first_at_,              0x48B48C)
FUNC(obj_find_first_at_tile_,         0x48B5A8) // <eax>(int elevation<eax>, int tile<edx>)
FUNC(obj_find_next_,                  0x48B41C)
FUNC(obj_find_next_at_,               0x48B510)
FUNC(obj_find_next_at_tile_,          0x48B608)
FUNC(obj_is_a_portal_,                0x49D140)
FUNC(obj_lock_is_jammed_,             0x49D410)
FUNC(obj_move_to_tile_,               0x48A568) // int aObj<eax>, int aTile<edx>, int aElev<ebx>
FUNC(obj_new_,                        0x489A84) // int aObj*<eax>, int aPid<ebx>
FUNC(obj_new_sid_inst_,               0x49AAC0)
FUNC(obj_outline_object_,             0x48C2B4)
FUNC(obj_pid_new_,                    0x489C9C)
FUNC(obj_remove_from_inven_,          0x49B73C)
FUNC(obj_remove_outline_,             0x48C2F0)
FUNC(obj_save_dude_,                  0x48D59C)
FUNC(obj_scroll_blocking_at_,         0x48BB44)
FUNC(obj_set_light_,                  0x48AC90) // <eax>(int aObj<eax>, signed int aDist<edx>, int a3<ecx>, int aIntensity<ebx>)
FUNC(obj_shoot_blocking_at_,          0x48B930)
FUNC(obj_sight_blocking_at_,          0x48BB88)
FUNC(obj_top_environment_,            0x48B304)
FUNC(obj_turn_off_,                   0x48AE68) // int aObj<eax>, int ???<edx>
FUNC(obj_unjam_lock_,                 0x49D480)
FUNC(obj_use_book_,                   0x49B9F0)
FUNC(obj_use_power_on_car_,           0x49BDE8)
FUNC(object_under_mouse_,             0x44CEC4)
FUNC(palette_fade_to_,                0x493AD4)
FUNC(palette_init_,                   0x493A00)
FUNC(palette_set_to_,                 0x493B48)
FUNC(partyMemberCopyLevelInfo_,       0x495EA8)
FUNC(partyMemberGetAIOptions_,        0x4941F0)
FUNC(partyMemberGetCurLevel_,         0x495FF0)
FUNC(partyMemberIncLevels_,           0x495B60)
FUNC(partyMemberPrepItemSaveAll_,     0x495140)
FUNC(partyMemberPrepLoad_,            0x4947AC)
FUNC(partyMemberRemove_,              0x4944DC)
FUNC(partyMemberSaveProtos_,          0x495870)
FUNC(pause_for_tocks_,                0x4C937C)
FUNC(pc_flag_off_,                    0x42E220)
FUNC(pc_flag_on_,                     0x42E26C)
FUNC(pc_flag_toggle_,                 0x42E2B0)
FUNC(perform_withdrawal_end_,         0x47A558)
FUNC(perkGetLevelData_,               0x49678C)
FUNC(perk_add_,                       0x496A5C)
FUNC(perk_add_effect_,                0x496BFC)
FUNC(perk_add_force_,                 0x496A9C)
FUNC(perk_can_add_,                   0x49680C)
FUNC(perk_description_,               0x496BB4)
FUNC(perk_init_,                      0x4965A0)
FUNC(perk_level_,                     0x496B78)
FUNC(perk_make_list_,                 0x496B44)
FUNC(perk_name_,                      0x496B90)
FUNC(perk_skilldex_fid_,              0x496BD8)
FUNC(perks_dialog_,                   0x43C4F0)
FUNC(pick_death_,                     0x41060C)
FUNC(pip_back_,                       0x497B64)
FUNC(pip_print_,                      0x497A40)
FUNC(pipboy_,                         0x497004)
FUNC(process_bk_,                     0x4C8BDC)
FUNC(protinst_use_item_,              0x49BF38)
FUNC(protinst_use_item_on_,           0x49C3CC)
FUNC(proto_dude_update_gender_,       0x49F984)
FUNC(proto_list_str_,                 0x49E758)
FUNC(proto_ptr_,                      0x4A2108) // eax - PID, edx - int** - pointer to a pointer to a proto struct
FUNC(pushLongStack_,                  0x46736C)
FUNC(qsort_,                          0x4F05B6)
FUNC(queue_add_,                      0x4A258C)
FUNC(queue_clear_type_,               0x4A2790)
FUNC(queue_explode_exit_,             0x4A2830)
FUNC(queue_find_,                     0x4A26A8)
FUNC(queue_find_first_,               0x4A295C)
FUNC(queue_find_next_,                0x4A2994)
FUNC(queue_leaving_map_,              0x4A2920)
FUNC(queue_next_time_,                0x4A2808)
FUNC(queue_remove_this_,              0x4A264C)
FUNC(rect_clip_,                      0x4C6AAC)
FUNC(rect_malloc_,                    0x4C6BB8)
FUNC(refresh_all_,                    0x4D7814)
FUNC(refresh_box_bar_win_,            0x4614CC)
FUNC(register_begin_,                 0x413AF4)
FUNC(register_clear_,                 0x413C4C)
FUNC(register_end_,                   0x413CCC)
FUNC(register_object_animate_,        0x4149D0) // int aObj<eax>, int aAnim<edx>, int delay<ebx>
FUNC(register_object_animate_and_hide_,          0x414B7C) // int aObj<eax>, int aAnim<edx>, int delay<ebx>
FUNC(register_object_call_,           0x414E98)
FUNC(register_object_change_fid_,     0x41518C) // int aObj<eax>, int aFid<edx>, int aDelay<ebx>
FUNC(register_object_funset_,         0x4150A8) // int aObj<eax>, int ???<edx>, int aDelay<ebx> - not really sure what this does
FUNC(register_object_light_,          0x415334) // <eax>(int aObj<eax>, int aRadius<edx>, int aDelay<ebx>)
FUNC(register_object_must_erase_,     0x414E20) // int aObj<eax>
FUNC(register_object_take_out_,       0x415238) // int aObj<eax>, int aHoldFrame<edx> - hold frame ID (1 - spear, 2 - club, etc.)
FUNC(register_object_turn_towards_,   0x414C50) // int aObj<eax>, int aTile<edx>
FUNC(remove_bk_process_,              0x4C8DC4)
FUNC(report_explosion_,               0x413144)
FUNC(reset_box_bar_win_,              0x4614A0)
FUNC(roll_random_,                    0x4A30C0)
FUNC(runProgram_,                     0x46E154) // eax - programPtr, called once for each program after first loaded - hooks program to game and UI events
FUNC(scr_exec_map_exit_scripts_,      0x4A69A0)
FUNC(scr_exec_map_update_scripts_,    0x4A67E4)
FUNC(scr_find_first_at_,              0x4A6524) // eax - elevation, returns spatial scriptID
FUNC(scr_find_next_at_,               0x4A6564) // no args, returns spatial scriptID
FUNC(scr_find_obj_from_program_,      0x4A39AC) // eax - *program - finds self_obj by program pointer (has nice additional effect - creates fake object for a spatial script)
FUNC(scr_find_sid_from_program_,      0x4A390C)
FUNC(scr_get_local_var_,              0x4A6D64)
FUNC(scr_new_,                        0x4A5F28) // eax - script index from scripts lst, edx - type (0 - system, 1 - spatials, 2 - time, 3 - items, 4 - critters)
FUNC(scr_ptr_,                        0x4A5E34) // eax - scriptId, edx - **TScript (where to store script pointer)
FUNC(scr_remove_,                     0x4A61D4)
FUNC(scr_set_ext_param_,              0x4A3B34)
FUNC(scr_set_local_var_,              0x4A6E58)
FUNC(scr_set_objs_,                   0x4A3B0C)
FUNC(scr_write_ScriptNode_,           0x4A5704)
FUNC(selectWindowID_,                 0x4B81C4)
FUNC(set_focus_func_,                 0x4C9438)
FUNC(set_game_time_,                  0x4A347C)
FUNC(setup_move_timer_win_,           0x476AB8)
FUNC(skill_check_stealing_,           0x4ABBE4)
FUNC(skill_dec_point_,                0x4AA8C4)
FUNC(skill_get_tags_,                 0x4AA508) // eax - pointer to array DWORD, edx - number of elements to read
FUNC(skill_inc_point_,                0x4AA6BC)
FUNC(skill_is_tagged_,                0x4AA52C)
FUNC(skill_level_,                    0x4AA558)
FUNC(skill_points_,                   0x4AA680)
FUNC(skill_set_tags_,                 0x4AA4E4) // eax - pointer to array DWORD, edx - number of elements to write
FUNC(skill_use_,                      0x4AAD08)
FUNC(skilldex_select_,                0x4ABFD0)
FUNC(soundDelete_,                    0x4AD8DC)
FUNC(soundGetPosition_,               0x4AE634)
FUNC(soundPlay_,                      0x4AD73C)
FUNC(soundPlaying_,                   0x4ADA84)
FUNC(soundSetCallback_,               0x4ADFF0)
FUNC(soundSetFileIO_,                 0x4AE2FC)
FUNC(soundVolume_,                    0x4ADE0C)
FUNC(sprintf_,                        0x4F0041)
FUNC(square_num_,                     0x4B1F04)
FUNC(stat_get_base_,                  0x4AF3E0)
FUNC(stat_get_base_direct_,           0x4AF408)
FUNC(stat_get_bonus_,                 0x4AF474)
FUNC(stat_level_,                     0x4AEF48) // &GetCurrentStat(void* critter, int statID)
FUNC(stat_pc_add_experience_,         0x4AFAA8)
FUNC(stat_pc_get_,                    0x4AF8FC)
FUNC(stat_pc_set_,                    0x4AF910)
FUNC(stat_recalc_derived_,            0x4AF6FC)
FUNC(stat_set_bonus_,                 0x4AF63C)
FUNC(stat_set_defaults_,              0x4AF6CC)
FUNC(strParseStrFromList_,            0x4AFE08)
FUNC(stricmp_,                        0x4DECE6)
FUNC(strncpy_,                        0x4F014F)
FUNC(switch_hand_,                    0x4714E0)
FUNC(talk_to_critter_reacts_,         0x447CA0)
FUNC(talk_to_translucent_trans_buf_to_buf_,      0x44AC68)
FUNC(text_curr_,                      0x4D58D4)
FUNC(text_font_,                      0x4D58DC)
FUNC(text_object_create_,             0x4B036C)
FUNC(tile_coord_,                     0x4B1674) // eax - tilenum, edx (int*) - x, ebx (int*) - y
FUNC(tile_dir_,                       0x4B1ABC)
FUNC(tile_dist_,                      0x4B185C)
FUNC(tile_num_,                       0x4B1754)
FUNC(tile_num_beyond_,                0x4B1B84)
FUNC(tile_num_in_direction_,          0x4B1A6C)
FUNC(tile_on_edge_,                   0x4B1D20)
FUNC(tile_refresh_display_,           0x4B12D8)
FUNC(tile_refresh_rect_,              0x4B12C0) // (int elevation<edx>, unkown<ecx>)
FUNC(tile_scroll_to_,                 0x4B3924)
FUNC(tile_set_center_,                0x4B12F8)
FUNC(trait_adjust_skill_,             0x4B40FC)
FUNC(trait_adjust_stat_,              0x4B3C7C)
FUNC(trait_get_,                      0x4B3B54)
FUNC(trait_init_,                     0x4B39F0)
FUNC(trait_level_,                    0x4B3BC8)
FUNC(trait_set_,                      0x4B3B48)
FUNC(trans_buf_to_buf_,               0x4D3704)
FUNC(trans_cscale_,                   0x4D3560)
FUNC(use_inventory_on_,               0x4717E4)
FUNC(win_add_,                        0x4D6238)
FUNC(win_clip_,                       0x4D75B0)
FUNC(win_delete_,                     0x4D6468)
FUNC(win_disable_button_,             0x4D94D0)
FUNC(win_draw_,                       0x4D6F5C)
FUNC(win_draw_rect_,                  0x4D6F80)
FUNC(win_enable_button_,              0x4D9474)
FUNC(win_fill_,                       0x4D6CC8)
FUNC(win_get_buf_,                    0x4D78B0)
FUNC(win_get_rect_,                   0x4D7950)
FUNC(win_get_top_win_,                0x4D78CC)
FUNC(win_height_,                     0x4D7934)
FUNC(win_hide_,                       0x4D6E64)
FUNC(win_line_,                       0x4D6B24)
FUNC(win_print_,                      0x4D684C)
FUNC(win_register_button_,            0x4D8260)
FUNC(win_register_button_disable_,    0x4D8674)
FUNC(win_register_button_sound_func_, 0x4D87F8)
FUNC(win_show_,                       0x4D6DAC)
FUNC(win_width_,                      0x4D7918)
FUNC(windowCheckRegion_,              0x4B6858)
FUNC(windowDisplayBuf_,               0x4B8EF0)
FUNC(windowDisplayTransBuf_,          0x4B8F64)
FUNC(windowGetBuffer_,                0x4B82DC)
FUNC(windowGetTextColor_,             0x4B6174)
FUNC(windowHide_,                     0x4B7610)
FUNC(windowOutput_,                   0x4B80A4)
FUNC(windowShow_,                     0x4B7648)
FUNC(windowWidth_,                    0x4B7734)
FUNC(windowWrapLineWithSpacing_,      0x4B8854)
FUNC(wmDrawCursorStopped_,            0x4C41EC)
FUNC(wmFindCurSubTileFromPos_,        0x4C0C00)
FUNC(wmInterfaceDrawSubTileRectFogged_,          0x4C40A8)
FUNC(wmInterfaceInit_,                0x4C2324)
FUNC(wmInterfaceRefresh_,             0x4C3830)
FUNC(wmInterfaceScrollTabsStart_,     0x4C219C)
FUNC(wmMapIsSaveable_,                0x4BFA64)
FUNC(wmMarkSubTileOffsetVisitedFunc_, 0x4C3434)
FUNC(wmMarkSubTileRadiusVisited_,     0x4C3550)
FUNC(wmMatchAreaContainingMapIdx_,    0x4C59A4)
FUNC(wmPartyInitWalking_,             0x4C1E54)
FUNC(wmPartyWalkingStep_,             0x4C1F90)
FUNC(wmRefreshInterfaceOverlay_,      0x4C50F4)
FUNC(wmSubTileMarkRadiusVisited_,     0x4C35A8)
FUNC(wmWorldMapFunc_,                 0x4BFE10)
FUNC(wmWorldMapLoadTempData_,         0x4BD6B4)
FUNC(xfclose_,                        0x4DED6C)
FUNC(xfeof_,                          0x4DF780)
FUNC(xfgetc_,                         0x4DF22C)
FUNC(xfgets_,                         0x4DF280)
FUNC(xfilelength_,                    0x4DF828)
FUNC(xfopen_,                         0x4DEE2C)
FUNC(xfputc_,                         0x4DF320)
FUNC(xfputs_,                         0x4DF380)
FUNC(xfread_,                         0x4DF44C)
FUNC(xfseek_,                         0x4DF5D8)
FUNC(xftell_,                         0x4DF690)
FUNC(xfwrite_,                        0x4DF4E8)
FUNC(xremovepath_,                    0x4DFAB4)
FUNC(xrewind_,                        0x4DF6E4)
FUNC(xungetc_,                        0x4DF3F4)
FUNC(xvfprintf_,                      0x4DF1AC)

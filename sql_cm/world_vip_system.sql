DELETE FROM `mangos_string` WHERE entry BETWEEN 1400 AND 1411;
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1400', '你的当前 VIP 等级为： %i.');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1401', '你的当前 梦境币 为 : %i.');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1402', '玩家: %s (账号: %s) 的 VIP 等级设置为 : %i.');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1403', '设置 VIP 等级失败！请检查你的输入');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1404', '玩家: %s (账号: %s) 的 梦境币 设置为 : %i.');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1405', '设置 梦境币 失败！请检查你的输入');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1406', '账号: %s 的 VIP 等级设置为：%i, 梦境币 设置为: %i.');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1407', '设置 VIP 等级 和 梦境币 错误，请检查你的输入.');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1408', '你为玩家: %s (账号: %s) 添加了 %i 个 梦境币, 该玩家当前 梦境币 为: %i.');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1409', '增加 梦境币 失败！请检查你的输入');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1410', '|cFF90EE90[梦境之末]:|r 恭喜你, 你获得了 |cFF90EE90%i|r 个 |cFF90EE90[梦境币]|r.(在线奖励)');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1411', '|cFF90EE90[梦境之末]:|r 恭喜你, 你获得了 |cFF90EE90%i|r 点 |cFF90EE90[经验值]|r |cFF90EE90%i|r ,个 |cFF90EE90[金币]|r(在线奖励)');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1414', '|cFFFF0000你尚未学习该专业技能！|r');

UPDATE `mangos_string` SET `content_default`='|cffffff00[|c00077766[温馨提示:]|cffffff00]: |cFFF222FF%s|r' WHERE (`entry`='1300');

INSERT INTO `command` (`name`, `help`) VALUES ('vip', '语法: .vip\r\n\r\n查看你的当前 VIP 等级.');
INSERT INTO `command` (`name`, `help`) VALUES ('point', '语法: .point\r\n\r\n查看你的当前 梦境币.');
INSERT INTO `command` (`name`, `security`, `help`) VALUES ('setvip', '3', '语法: .setvip [#角色名] VIP等级\r\n\r\n设置指定角色的 VIP 等级。 当#角色名没有指定时， 设置当前选中角色的VIP等级。');
INSERT INTO `command` (`name`, `security`, `help`) VALUES ('setpoint', '3', '语法: .setpoint [#角色名] 梦境币\r\n\r\n设置指定角色的梦境币. 当 #角色名 没有指定时, 设置当前选中角色的 梦境币. ');
INSERT INTO `command` (`name`, `security`, `help`) VALUES ('modify point', '3', '语法: .modify point [#角色名] 梦境币\r\n\r\n为指定角色增加 梦境币 . 当 #角色名 没有指定时 为当前选中角色增加梦境币.\r\n梦境币的值 可以为负数.');
INSERT INTO `command` (`name`, `security`, `help`) VALUES ('account set vip', '3', '语法: .account set vip [#账号] VIP等级 梦境币\r\n\r\n设置指定账号的 VIP 等级和 梦境币. 当 #账号 没有指定时, 设置当前选择角色的 VIP 等级和 梦境币.')

alter table chess.account0 add userright int DEFAULT 0 NOT NULL COMMENT '用户权限';
alter table chess.account0 add posrmb int DEFAULT 0 NOT NULL COMMENT '第一次触发的充值额度';
alter table chess.account0 add postime int DEFAULT 0 NOT NULL COMMENT '第一次触发的时间';
alter table chess.account0 add welcount int DEFAULT 0 NOT NULL COMMENT '福利局数';
alter table chess.account0 add weltime int DEFAULT 0 NOT NULL COMMENT '福利时间';


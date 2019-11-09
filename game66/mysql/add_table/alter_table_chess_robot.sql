


alter table chess.robot add column logintype int DEFAULT 0 NOT NULL COMMENT '登录类型 0 全天登录 1 按照时段登录' after gametype;

alter table chess.robot add column roomid int DEFAULT 0 NOT NULL COMMENT '房间id' after gametype;

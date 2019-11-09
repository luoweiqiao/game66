INSERT INTO `chess_sysdata`.`sysconfig` (`sckey`, `value`, `mean`, `mkfile`, `vtype`) VALUES ('unionpayrecharge', '{\"data\":{\"action\":\"unionpay\",\"recharge\":1100,\"status\":0}}', '银联充值', '0', 'json');

INSERT INTO `chess_sysdata`.`sysconfig` (`sckey`, `value`, `mean`, `mkfile`, `vtype`) VALUES ('wechatpayrecharge', '{\"data\":{\"action\":\"wechatpay\",\"recharge\":1100,\"status\":0}}', '微信', '0', 'json');
INSERT INTO `chess_sysdata`.`sysconfig` (`sckey`, `value`, `mean`, `mkfile`, `vtype`) VALUES ('alipayrecharge', '{\"data\":{\"action\":\"alipay\",\"recharge\":1100,\"status\":0}}', '支付宝', '0', 'json');
INSERT INTO `chess_sysdata`.`sysconfig` (`sckey`, `value`, `mean`, `mkfile`, `vtype`) VALUES ('otherpayrecharge', '{\"data\":{\"action\":\"otherpay\",\"recharge\":1100,\"status\":0}}', '其他支付', '0', 'json');
INSERT INTO `chess_sysdata`.`sysconfig` (`sckey`, `value`, `mean`, `mkfile`, `vtype`) VALUES ('qqpayrecharge', '{\"data\":{\"action\":\"qqpay\",\"recharge\":1100,\"status\":0}}', 'QQ', '0', 'json');
INSERT INTO `chess_sysdata`.`sysconfig` (`sckey`, `value`, `mean`, `mkfile`, `vtype`) VALUES ('wcscanpayrecharge', '{\"data\":{\"action\":\"wechatscanpay\",\"recharge\":1100,\"status\":0}}', '微信扫码', '0', 'json');
INSERT INTO `chess_sysdata`.`sysconfig` (`sckey`, `value`, `mean`, `mkfile`, `vtype`) VALUES ('jdpayrecharge', '{\"data\":{\"action\":\"jdpay\",\"recharge\":1100,\"status\":0}}', '京东支付', '0', 'json');



select * from `chess_sysdata`.`sysconfig` where sckey = "unionpayrecharge";
select * from `chess_sysdata`.`sysconfig` where sckey = "wechatpayrecharge";
select * from `chess_sysdata`.`sysconfig` where sckey = "alipayrecharge";
select * from `chess_sysdata`.`sysconfig` where sckey = "otherpayrecharge";
select * from `chess_sysdata`.`sysconfig` where sckey = "qqpayrecharge";
select * from `chess_sysdata`.`sysconfig` where sckey = "wcscanpayrecharge";
select * from `chess_sysdata`.`sysconfig` where sckey = "jdpayrecharge";

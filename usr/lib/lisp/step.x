(File step.l)
(clear-input-buffer lambda drain)
(continue-evaluation lambda funcallhook evalhook eq cond)
(evalhook* lambda quote funcall-evalhook*)
(funcallhook* lexpr quote funcall-evalhook* cons |1-| + cdr <& do minusp eq cond sub1 listify arg let)
(stephelpform lambda terpri terpr patom)
(funcall-evalhook* lambda bigp zerop continue-evaluation clear-input-buffer stephelpform list symbolp dtpr read reset step *break break debug sstatus go tyi drain let terpr sub1 prog terpri evalhook quote princ null numberp or |1+| tyo print patom greaterp > cdr * do print* setq car memq eq atom not and cond)
(step nlambda *rset sstatus quote eq setq car null or cond)

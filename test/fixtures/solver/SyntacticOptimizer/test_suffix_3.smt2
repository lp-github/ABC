(set-logic QF_S)

(declare-fun a () String)

(assert (= (concat a "a") "aa"))

(check-sat)

(set-logic QF_S)

(declare-fun var_abc () String)

(assert (= var_abc (lastIndexOf /(abcbc|deb+)/ "b")))

(check-sat)


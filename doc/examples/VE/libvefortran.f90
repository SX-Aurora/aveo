SUBROUTINE SUB1(x, ret)
  implicit none
  INTEGER, INTENT(IN) :: x
  INTEGER, INTENT(OUT) :: ret
  ret = x + 1
END SUBROUTINE SUB1

INTEGER FUNCTION FUNC1(x, y)
  implicit none
  INTEGER, VALUE :: x, y
  FUNC1 = x + y
END FUNCTION FUNC1

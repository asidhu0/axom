!
! Test code generated by the strings test
!
program tester
  use fruit
  use iso_c_binding
  use strings_mod
  implicit none
  logical ok

  character(4)  status

  call init_fruit

  call test_charargs
  call test_functions

  call fruit_summary
  call fruit_finalize

  call is_all_successful(ok)
  if (.not. ok) then
     call exit(1)
  endif

contains

  subroutine test_charargs

    character(30) str
    character ch

    call set_case_name("test_charargs")

    call pass_char("w")

    ! character(*) function
    str = 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
    call pass_char_ptr(dest=str, src="bird")
    call assert_true( str == "bird")

    ch = return_char()
    call assert_true( ch == "w")

  end subroutine test_charargs


  subroutine test_functions

    character(30) str

    call set_case_name("test_functions")

    ! character(*) function
    str = 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
    str = get_char1()
    call assert_true( str == "bird")

    ! character(30) function
    str = 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
    str = get_char2()
    call assert_true( str == "bird")

    ! string_result_as_arg
    str = 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
    call get_char3(str)
    call assert_true( str == "bird")
 
!--------------------------------------------------

    ! character(*) function
    str = 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
    str = get_string1()
    call assert_true( str == "dog")

    ! character(30) function
    str = 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
    str = get_string2()
    call assert_true( str == "dog")

    ! string_result_as_arg
    str = 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
    call get_string3(str)
    call assert_true( str == "dog")
 
!--------------------------------------------------

    call accept_string_const_reference("cat")
!    call assert_true( rv_char == "dog")

    str = "cat"
    call accept_string_reference(str)
    call assert_true( str == "catdog")


  end subroutine test_functions

end program tester

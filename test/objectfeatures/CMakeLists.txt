IF(WIN32)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
ENDIF(WIN32)

VIGRA_ADD_TEST(test_objectfeatures test.cxx LIBRARIES vigraimpex)
VIGRA_ADD_TEST(test_stand_alone_acc_chain stand_alone_acc_chain.cxx)
VIGRA_COPY_TEST_DATA(of.gif)


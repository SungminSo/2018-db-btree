이 코드는 C로 작성되었으며 Makefile이 아닌 .c 파일(bpt3.c, bpt3.h)과 컴파일된 파일(bpt3)로 제출되었습니다.

이전의 코드와 달라진 점
1. table_id가 도입되었다. table_id가 의미하는 바는 서로 다른 파일에 부여되는 파일의 id값이다. 예를 들면 test1.db의 table_id는 3, test2.db의 table_id는 4 와 같이 table_id값이 파일을 열때 table_open함수를 통해 부여된다. table_id는 최대 10개까지 사용가능하다. 따라서 서로 다른 header page, free page, root page가 최대 10개까지 사용가능하며 이는 배열로 구현되어 table_id값을 index로 사용하여 호출 및 사용된다.

2.index에서 File Manager API를 통해 곧바로 disk를 읽어왔던 기존의 방식과는 달리 buffer layer를 도입하므로써 index layer의 요청에 따라 disk의 정보는 buffer를 거쳐 index layer로 전달된다.
- 이를 위해서 기존의 index layer에서 사용되던 File Manager API들은 Buffer Manager API들로 교체되었고 Buffer Manager API 내에서 File Manager API들이 사용된다.

--> index layer 내에서
      file_alloc_page()  --> buf_ alloc_page(int table_id)
      file_free_page(pagenum_t pagenum) --> buf_free_page(int table_id, pagenum_t pagenum)
      file_read_page(pagenum_t pagenum, page_t * dest) --> buf_get_page(int table_id, pagenum_t offset, page_t * dest)
      file_write_page(pagenum_t pagenum, page_t * src) --> buf_put_page(int table_id, pagenum_t offset, page_t * src)
      로 교체되어 사용된다.

--> Buffer Manager API 내에서
      buf_alloc_page 함수에서 file_write_page 함수,  file_read_page 함수, file_alloc_page 함수 사용
      buf_free_page 함수에서 file_write_page 함수, file_read_page 함수 사용
      buf_get_page 함수에서 file_read_page 함수 사용
      buf_put_page 함수에서 file_write_page 함수 사용
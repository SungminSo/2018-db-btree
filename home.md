Database System assignment02

1.Possible call path of the insert/delete operation
	insert

	1.입력받은 값이 이미 존재하는 tree 안에 존재하는 경우(값이 중복인 경우) 
	-> scanf("%" PRId64 "%s", &input, string) 
	-> insert(input,string) 
	-> find(key)(key = input) 
	-> find_leaf(key) 
	-> return -1
	-> print_tree()

	2.입력받은 값이 중복이 아니지만 존재하는 tree가 없는 경우 

	-> scanf("%" PRId64 "%s",  &input, string) 
	-> insert(input,string) 
	-> find(key)(key = input) 
	-> find_leaf(key) 
	-> make_record(key, value)(pointer(record) 생성) 
	-> start_new_tree(key,pointer) 
	-> make_page()  
	-> return 0
	-> print_tree()

	3.입력받은 값이 중복이 아니고 존재하는 tree가 있는 경우 + 입력받은 값이 들어갈 적절한 위치의 노드(leaf_node)의 크기가 정해진 노드의 크기(LEAF_ORDER - 1) 보다 작은 경우 
	-> scanf("%" PRId64 "%s",  &input, string) 
	-> insert(input,string) 
	-> find(key)(key = input) 
	-> find_leaf(key) 
	-> make_record(key, value)(pointer(record) 생성) 
	-> find_leaf(key) 
	-> insert_into_leaf(leaf,key,pointer) 
	-> return 0
	-> print_tree()

	4.입력받은 값이 중복이 아니고 존재하는 tree가 있는 경우 + 입력받은 값이 들어갈 적절한 위치의 노드(leaf_node)의 크기가 정해진 노드의 크기(LEAF_ORDER - 1)와 같은 경우(split 발생) + parent 노드(internal 노드)가 존재하지 않는 경우 
	-> scanf("%" PRId64 "%s",  &input, string) 
	-> insert(input,string) 
	-> find(key)(key = input) 
	-> find_leaf(key) 
	-> make_record(key, value)(pointer(record) 생성) 
	-> find_leaf(key) 
	-> insert_into_leaf_after_splitting(leaf,key,pointer) 
	-> make_page()
	-> cut(LEAF_ORDER - 1) 
	-> (*) insert_into_parent(leaf,new_key,new_leaf)(new_key = key) 
	-> insert_into_new_root(child,key,new_child)(child = leaf,new_child = new_leaf) (*,1) 
	-> return 0
	-> print_tree()

        5.입력받은 값이 중복이 아니고 존재하는 tree가 있는 경우 + 입력받은 값이 들어갈 적절한 위치의 노드(leaf)의 크기가 정해진 노드의 크기(LEAF_ORDER - 1)와 같은 경우(split 발생) + parent 노드(internal 노드)가 존재하는 경우 + parent 노드의 크기가 정해진 크기(INTERNAL_ORDER - 1)보다 작은 경우
        -> scanf("%" PRId64 "%s",  &input, string) 
	-> insert(input,string) 
	-> find(key)(key = input) 
	-> find_leaf(key) 
	-> make_record(key, value)(pointer(record) 생성) 
	-> find_leaf(key) 
	-> insert_into_leaf_after_splitting(leaf,key,pointer) 
	-> make_page()
	-> cut(LEAF_ORDER - 1) 
	-> insert_into_parent(leaf,new_key,new_leaf)(new_key = key) 
	-> get_left_index(parent,child)(left_index 값) 
	-> insert_into_node(parent,left_index,new_key,new_child) 
	-> return 0
        -> print_tree()

	6.입력받은 값이 중복이 아니고 존재하는 tree가 있는 경우 + 입력받은 값이 들어갈 적절한 위치의 노드(leaf_node)의 크기가 정해진 노드의 크기LEAF_ORDER - 1)와 같은 경우(split 발생) + parent 노드(internal 노드)가 존재하는 경우 + parent 노드의 크기가 정해진 크기(INTERNAL_ORDER - 1)와 같은 경우(split 발생) 
	-> scanf("%" PRId64 "%s",  &input, string) 
	-> insert(input,string) 
	-> find(key)(key = input) 
	-> find_leaf(key) 
	-> make_record(key, value)(pointer(record) 생성) 
	-> find_leaf(key) 
	-> insert_into_leaf_after_splitting(leaf,key,pointer) 
	-> make_page()
	-> cut(LEAF_ORDER - 1) 
	-> insert_into_parent(leaf,new_key,new_leaf)(new_key = key) 
	-> get_left_index(parent,child)(left_index 값) 
	-> insert_into_node_after_splitting(parent,left_index,new_key,new_child) 
	-> make_page()
        -> cut(INTERNAL_ORDER)
	-> insert_into_parent(root,old_node,k_prime,new_node)(k_prime = new_node의 첫번째 값) (*) 
	-> 5번의 (*)~(*,1) 상황에 발생할 때까지(최악의 경우 최상위 root 노드까지) (*)~(*) 과정 반복) 
	-> return 0
	-> print_tree()


	delete

	1.입력받은 값이 tree에 존재하지 않을 때 
	-> scanf("%" PRId64 "", &input) 
	-> delete(input) 
	-> find_leaf(key)(key = input) 
	-> find(key) 
	-> return -1
	-> print_tree()
	
        2.입력받은 값이 tree에 존재할 때 + 해당 값이 위치한 노드가 root 노드이고 노드에 있는 값이 2개 이상이었을 때 
	-> scanf("%" PRId64 "", &input) 
	-> delete(input) 
	-> find_leaf(key)(key = input) 
	-> find(key) 
	-> delete_entry(root,key_leaf,key,key_record) 
	-> remove_entry_from_node(page,key,key_value) 
	-> adjust_root() 
	->  return 0 
	-> print_tree()
	
         3.입력받은 값이 tree에 존재할 때 + 해당 값이 위치한 노드가 root 노드이고 노드에 값이 1개였을 때 
	-> scanf("%" PRId64 "", &input) 
	-> delete(input) 
	-> find_leaf(key)(key = input) 
	-> find(key) 
	-> delete_entry(root,key_leaf,key,key_record) 
	-> remove_entry_from_node(page,key,key_value) 
	-> adjust_root() 
	-> return new_root(new_root = null) (d*,3) 
	-> print_tree()
	
        4.입력받은 값이 tree에 존재할 때 + 해당 값이 위치한 노드가 root 노드가 아니고 해당 노드의 크기가 최소 크기보다 크거나 같을 때 
	-> scanf("%" PRId64 "", &input) 
	-> delete(input) 
	-> find_leaf(key)(key = input) 
	-> find(key) 
	-> delete_entry(root,key_leaf,key,key_record) 
	-> remove_entry_from_node(page,key,key_value) 
	->  return 0
	-> print_tree()
	
        5.입력받은 값이 tree에 존재할 때 + 해당 값이 위치한 노드가 root 노드가 아니고 해당 노드의 크기가 최소 크기보다 작을 때 + 왼쪽의 이웃한(해당 노드가 가장 왼쪽 노드인 경우 오른쪽의) 노드와의 값 개수 합이 capacity보다 작을 때(coalescence 발생) 
	-> scanf("%" PRId64 "", &input) 
	-> delete(input) 
	-> find_leaf(key)(key = input) 
	-> find(key) 
	-> delete_entry(root,key_leaf,key,key_record) 
	-> remove_entry_from_node(page,key,key_value) 
        -> get_neighbor_index(page)
        -> merge_nodes(page, neighbor, neighbor_index, k_prime)
	->  return root
	-> print_tree()


2.Detail flow of the structure modification(split, merge)
	
structure modification이 발생하는 이유: 하나의 노드에 order보다는 적고 order/2보다는 크거나 같은 수의 key값들을 가지고 있으려고 하는 b+ tree의 속성때문.

	split
	1번의 insert call path들을 통해 split이 일어난 후 세 가지 경우들을 알 수 있다.
	 (1)parent node가 존재하는 않는 경우(=해당 node가 root node)
	 (2)parent node의 크기가 정해진 크기보다 작을 경우
	 (3)parent node의 크기가 정해진 크기와 같아 internal(혹은 root) node에서도 split이 일어나야하는 경우
	 (split이 일어나는 조건)
	 - 중복되지 않는 값이 입력되야함
	 - find_leaf 함수를 통해 찾은 적절한 위치의 leaf node의 num_keys가 order - 1이어야함(즉, 입력받은 값을 입력할 경우 num_keys == order 인 경우)
	-> insert_into_leaf_after_splitting 함수 호출
	-> int * temp_keys에 order만큼 동적할당(새로운 node의 key 개수가 될것)
	-> int ** temp_pointers에 order만큼 동적할당(새로운 node의 pointer 개수가 될것)
	-> 위치의 값이 order - 1보다 작고 해당 위치의 값이 key값보다 작을 때 i++하면서 key값이 들어갈 적절한 위치 찾음(값이 작을수록 왼쪽(index가 낮은 쪽)에 있다고 가정 -> 지속적으로 지켜지므로 보장됨)
	-> (*copy) 적절한 위치가 j라고 할때 temp_keys와 temp_pointers에 j칸을 제외한 나머지 칸들에 기존의 key들과 pointer들을 복사(j미만이 위치에는 key값들과 pointer값들 제자리에, j 이상의 위치에는 key값들과 pointer값들이 한칸 뒤로 복사됨.)
	-> temp_keys와 temp_pointers의 j칸에 입력받았던 key와 pointer 넣음
	-> 기존 node의 num_keys값을 0으로 바꿈
	-> split = order - 1의 절반 혹은 order - 1의 절반 + 1
	-> split개만큼은 temp_keys와 temp_pointers에서 다시 기존 node로 복사
	-> temp_keys와 temp_pointers의 나머지(order - split)값들은 make_leaf 함수로 만들어두었던 new node에 복사
	-> temp_keys와 temp_pointers 메모리 해제 (*)
	-> new node는 기존의 leaf node가 가리키던 다른 node를 가리키는 포인터를 가지고, 기존의 leaf node는 새로운 node를 가리키게 됨
	-> 각각의 node의 나머지 칸들의 pointer들은 null값을 할당
	-> new node의 parent를 기존의 leaf node와 같게 할당
	-> new_key는 new node의 첫번째 값으로 할당
	-> insert_into_parent 함수 호출
	-> parent node가 없었다면 make_node 함수로 새로운 노드를 만들어 pointers[0]에는 기존의  leaf node를, pointers[1]에는 new_leaf node를 할당((1)에 해당하는 flow)
	-> parent node에서 기존의 leaf node의 위치를 찾아내서 그 값을 left_index에 할당
	-> parent node의 num_keys가 order - 1 보다 작았다면 insert_into_node 함수 호출
	-> left_index이후의 값(node)들은 한칸씩 뒤로 밀어내고 new_leaf node를 가리키는 pointer와 node의 첫번째 값 추가((2)에 해당하는 flow)
	-> parent node의 num_keys가 order - 1 보다 작지 않다면 insert_into_node_after_splitting 함수 호출
	-> temp_pointers에는 order + 1만큼,temp_keys에는 order만큼 동적할당
	-> temp_pointers와 temp_keys에 기존 parent node의 pointers와 key를 앞의 (*copy) ~ (*) 부분과 마찬가지의 방식으로 복사
	->각각의 node의 나머지 칸들의 pointer들은 null값을 할당
	-> new node의 parent를 기존의 node와 같게 할당
	-> k_prime은 new node의 첫번째 값으로 할당
	-> insert_into_parent 함수 호출
	-> (2)상황이 발생할 때까지(최악의 경우 최상위 root node 까지) insert_into_parent 함수 반복 호출
	 merge
	 1번의 delete call path들을 통해 merge가 일어나는 두 가지 경우들을 알 수 있다.
	 	(1)coalesce 	->두 node의 num_keys를 합쳐도 order(capacity)를 넘지 않는 경우
	 	(2)redistribute ->두 node를 합칠 정도는 아니지만 한쪽의 num_keys가 order/2 넘지못해서 하나의 key와 pointer를 넘겨주는 경우
	 -> delete 함수 호출
	 -> node * key_leaf와 record * key_record가 각각 find,find_leaf 함수 호출해서 입력받은 값이 tree내에 존재하는지 확인
	 -> 존재하지 않는다면 return root, 존재한다면 delete_entry 함수 호출
	 -> key_leaf, key, key_record에 대해서 remove_entry_from_node함수 호출
	 -> tree 내에서는 key의 크기순으로 정렬되어 있다는 것이 보장되므로 node내의 다른 key값들과 입력받았던 key값의 크기비교로 node에서 해당 key의 위치가 어디인지 찾음
	 -> 해당 위치 다음부터 num_keys까지 오른쪽의 key값을 왼쪽으로 한칸씩 땡겨서 덮어쓰기
	 -> 입력받았던 값은 덮어쓰기에 의해 지워지고 다른 값들은 그대로 존재, num_keys값은 1 줄어듦
	 -> 해당 node가 leaf node라면 num_pointers의 크기를 num_keys만큼, leaf node가 아니라면 num_pointers + 1만큼 할당해줌(node의 pointer 개수가 leaf node의 pointer 개수보다 1개 커야하기 때문)
	 -> 해당 위치 다음부터 num_pointers까지 오른쪽의 pointer값을 왼쪽으로 한칸씩 덮어쓰기(지우고자 했던 key값과 pointer값 삭제 완료)
	 -> num_keys + 1번째부터 order - 1까지 사용하지 않는 pointer들의 값을 null로 할당
	 -> 다시 remove_entry_from_node 함수로 돌아와서 값을 지웠던 node가 root node인지 아닌지 판단해서 root node라면 adjust_root 함수를 호출해야 하지만 만약 그렇다면 merge는 일어나지 않으므로 현재는 skip
	 -> 만약 해당 node가 leaf node 라면 min_keys에 cut(order - 1)만큼을 그렇지 않다면 cut(order) -1 만큼을 할당(order가 짝수라면 cut(order - 1)이 cut(order) - 1 보다 1 크고, 홀수라면 둘의 값을 같음)
	 -> min_keys는 b+ tree 속성에 의해서 각 node마다 가져야하는 keys의 최소개수
	 -> 만약 해당 node의 num_keys가 min_keys보다 크거나 같다면 return root이지만 만약 그렇다면 merge는 일어나지 않으므로 현재는 skip
	 -> 이제 merge가 발생하는 상황, coalescence가 발생해야 할지 redistribution이 발생해야 할지 판단하기 위해 해당 node와 merge가 수행될 이웃 node 탐색
	 -> get_neighbor_index 함수를 호출해서 해당 node의 parent node에서 neighbot node의 위치가 어디인지 찾음
	 -> 만약 해당 node가 parent node의 값들 중 가장 왼쪽 node가 아니었다면 neighbor node는 해당 노드의 왼쪽 node이고, 가장 왼쪽 node라면 neighbor node는 해당 노드의 오른쪽 node
	 -> k_prime는 두 node(해당 node와 neighbor node) 중 오른쪽에 있는 것의 첫번째 값(해당 node가 parent node에서 가장 왼쪽 node라면 k_prime은 해당 node의 첫번째 값을 가리키지만 이런 경우 merge가 일어나면 해당 node와 neighbor node를 swap하기 때문에 둘 중 오른쪽 node의 첫번째 값으로 생각한다.)
	 -> capacity에는 해당 node가 leaf node라면 order, 아니라면 order - 1을 할당
	 -> (*) 해당 node의 num_keys와 neighbor node의 num_keys를 더한값을 capacity와 비교
	 -> (*)에서 만약 두 node의 num_keys를 더한값이 capacity보다 작다면 coalescence를 실행해야 하므로 coalesce_nodes 함수 호출
	 -> coalesce_nodes 함수에서 우선 해당 node와 neighbor node의 위치관계를 확인해 왼쪽에 있는 것을 neighbor, 오른쪽에 있는 것을 n이라 하고 n의 값들을 neighbor로 옮김
	 -> 옮길때는 neighbor->keys[i]에 n->keys[j] 값을 옮기고, i는 기존의 neighbor->num_keys이후부터, j는 0부터 n->num_keys까지 neighbor->num_keys++, n->num_keys--해주면서 반복
	 -> 마지막에 pointer의 개수는 key의 개수보다 항상 1 크므로 마지막 pointer은 따로 옮겨줌
	 -> 옮겨진 key들의 parent를 neighbor로 지정
	 -> n과 neighbor가 leaf node일때와 그렇지 않을때 값을 옮기는 횟수가 1 차이나지만 과정은 다르지 않음
	 ->다시 delete_entry 함수를 호출하며 해당 노드에서의 변화가 parent node의 merge를 유발하지 않을때까지 위 과정 반복
	 -> (*)에서 만약 두 node의 num_keys를 더한값이 capacity보다 작지 않다면 redistribution을 실행해야 하므로 redistribute_nodes 함수 호출
	 -> deletion이 n node에서 발생했으므로 neighbor node에서 값 하나를 n node로 옮기면 됨
	 -> 해당 node가 parent node에서 가장 왼쪽 node가 아니면 neighbor node가 왼쪽, 해당 node(n)가 오른쪽
	 -> n의 key,pointer값들을 하나씩 오른쪽으로 옮김(leaf node가 아니라면 pointer가 하나 더 많으므로 전체를 옮기기 전에 가장 오른쪽 pointer를 따로 옮겨줌)
	 -> 그 후 neighbor의 가장 마지막 값의 key와 pointer를 n의 첫번째 칸에 옮기고 해당 node(또는 record)의 parent를 n으로 지정
	 -> 만약 해당 node(n)가 parent node에서 가장 왼쪽의 node라면 neighbor node의 첫번째 값을 n의 마지막에 옮겨넣고 해당 node(또는 record)의 parent를 n으로 지정
	 -> neighbor node의 나머지 값들을 왼쪽으로 한칸씩 옮김


3.(Naive)designs or required changes for building on-disk b+ tree
	
	-bpt.c와 main.c 파일을 통해서 구현된 것은  in-memory b+ tree 이므로 이것을 on-disk b+tree 바꾸려면 in-memory b+ tree의 node에 offset을 지정해주어 disk의 특정한 위치에 저장될 수 있도록 변경해야 한다.
        in-memory b+ tree와는 다르게 on-disk b+ tree에서는 header page가 존재해야 하며 header page에서는 root page의 offset과 다음에 사용될 free page의 offset과 사용된 전체 page의 개수를 서술해야 한다.
        in-memoryb+ tree에서의 node들은 internal page와 leaf page로 나눠야 하고 두 page의 공통된 멤버 변수를 가지고 있는 page_t 구조체가 필요하다.
	on-disk b+ tree의 작동 방식은 in-memory b+ tree와 매우 유사하며 매 연산 전에 page의 정보를 읽어오는 것과 연산이 끝난 후 page 정보를 disk에 저장하는 차이가 있다. 해당 process는 File Manager API를 이용했다.

       -header page, free page, internal page, leaf page, page_t 에 해당하는 구조체가 각각 존재한다. 

// Page_t
typedef struct page_t {
	pagenum_t offset;
	union {
		struct {
			pagenum_t parent_offset;
			bool is_leaf;
			int num_keys;
		};
		char page[PAGE_SIZE - 8];
	};

} page_t;

// leaf page
typedef struct leaf_page {
	pagenum_t offset;
	union {
		struct {
			pagenum_t parent_offset;	// Parent page
			bool is_leaf;				// yes->1
			int num_keys;				// Number of keys
			char reserved[96];		// Reserved memory (104-8) bytes;
			pagenum_t next_offset; 	// Right sibling page
			record pointers[LEAF_ORDER - 1];		// key(8) + value(120)
		};
		char page[PAGE_SIZE - 8];
	};
}leaf_page;

// Internal page
typedef struct in_page {
	pagenum_t offset;
	union {
		struct {
			pagenum_t parent_offset;				// Parent page
			bool is_leaf;							// no->0
			int num_keys;							// Number of keys
			pagenum_t offsets[INTERNAL_ORDER];		// Page offset(8)
			int64_t keys[INTERNAL_ORDER - 1];		// key(8)
		};
		char page[PAGE_SIZE - 8];
	};
} in_page;

// Free pages
typedef struct free_page {
	pagenum_t offset;
	union {
		struct {
			pagenum_t next_page_offset;			// Next free page offset
			char not_used[PAGE_SIZE - 16];			// Not used memory 4080 bytes
		};
		char page[PAGE_SIZE - 8];
	};
} free_page;

// Header page
typedef struct header_page {
	pagenum_t offset;
	union {
		struct {
			pagenum_t free_offset;		// Free page offset
			pagenum_t root_offset;		// Root page offset
			pagenum_t num_pages;		// Number of pages
			char reserved[4064];		// Reserved memory 4072 bytes
		};
		char page[PAGE_SIZE - 8];
	};
} header_page;

        각각의 구조체에서 in-memory와는 다르게 pointer 형식이 아니라 pagenum_t 형식의 변수를 사용하는 이유는 기존에 존재하는 파일에서 pointer 형식을 사용하여 자식 internal page 혹은 leaf page에 대한 정보를 가져오는 경우 잘못된 값을 읽어오거나 아예 읽어오지 못하기  때문이다. 따라서 각 page의 정보는 disk의 특정한 offset에 저장되어 있기 때문에 file manager API를 통하여 해당 offset부터 page의 일정한 크기인 4096 bytes만큼을 읽어오는 것으로 변환하였다.
	
	-node 구조체에 추가적으로 pin혹은 lock 등의 멤버변수를 추가해야 한다.
	현재는 single-thread로 동작하는 것만 고려하고 있지만 추후에는 multi-thread 역시 고려해야 하기 때문에 혼선과 충돌을 막기 위해서 pin과 lock 등의 변수들을 사용해야 한다.
	pin과 lock 등은 insert 시에는 0 혹은 null로 초기화를 하고 find 혹은 delete 함수 사용시 함수 호출 전 값을 조작하는 방식으로 구현을 할 생각이다.
	
        -merge를 delay 시킬 필요가 있다. 
	다루는 데이터의 양이 많아지면 delete가 실행될 때마다 조건 충족시 merge시킨다면 1-5에서 볼 수 있듯이 많은 횟수동안 반복적으로 parent node를 조작해야 한다. 
	또 multi-thread를 사용하는 경우 오랜시간동안 lock이 걸려야하기 때문에 사용가능 시간에 제약이 생긴다. 
	따라서 앞으로 구현할 on-disk b+ tree에서는 merge를 최대한 delay 시켜 해당 page에 값들이 모두 사라져 num_keys가 0이 되면 merge를 실행하도록 구현할 것이다. 
	따라서 현재 bpt.c에 있는 redistribute_nodes함수는 삭제하고 coalesce_nodes 함수는 변경해야 한다. 
	delete_entry 함수에서 뒷부분의 return 값들도 변경할 생각이다.
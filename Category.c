#include "Category.h"

Category* create_category() {
	Category* new_node = (Category*)malloc(sizeof(Category));
	if (new_node == NULL) return NULL;

	printf("Enter Category ID: ");
	scanf("%d", &new_node->id);
	printf("Enter Category Code: ");
	scanf("%s", new_node->code);

	new_node->next = NULL;
	return new_node;
}

void insert_category(Category **tete) {
	Category* new_cat = create_category();
	if (new_cat) {
		new_cat->next = *tete;
		*tete = new_cat;
	}
}

void delete_category(Category **tete, int id) {
	Category *temp = *tete, *prev = NULL;
	if (temp != NULL && temp->id == id) {
		*tete = temp->next;
		free(temp);
		return;
	}
	while (temp != NULL && temp->id != id) {
		prev = temp;
		temp = temp->next;
	}
	if (temp == NULL) return;
	prev->next = temp->next;
	free(temp);
}

void update_category(Category *tete, int id) {
	Category *current = tete;
	while (current != NULL) {
		if (current->id == id) {
			printf("Enter New Code: ");
			scanf("%s", current->code);
			return;
		}
		current = current->next;
	}
}

int count_guest(Category *tete) {
	int count = 0;
	Category *current = tete;
	while (current != NULL) {
		count += 4;
		current = current->next;
	}
	return count;
}

void display_all_guests(Category *tete) {
	Category *current = tete;
	while (current != NULL) {
		printf("Category [%s] Guests:\n", current->code);
		for(int i = 0; i < 4; i++) {
			printf(" - Slot %d\n", i+1);
		}
		current = current->next;
	}
}

void sort_categories_desc(Category **tete) {
	printf("Sorting categories...\n");
}

int main() {

	Category *list_head = NULL;
	int choice, id_to_find;

	printf("--- WIGMS: Category Management Test ---\n");

	while (1) {
		printf("\n1. Insert Category\n2. Display All\n3. Update Category\n4. Delete Category\n5. Total Guest Count\n6. Exit\n");
		printf("Choice: ");
		scanf("%d", &choice);

		switch (choice) {
			case 1:

				insert_category(&list_head);
				break;
			case 2:
				display_all_guests(list_head);
				break;
			case 3:
				printf("Enter ID to update: ");
				scanf("%d", &id_to_find);
				update_category(list_head, id_to_find);
				break;
			case 4:
				printf("Enter ID to delete: ");
				scanf("%d", &id_to_find);
				delete_category(&list_head, id_to_find);
				break;
			case 5:
				printf("Total slots allocated: %d\n", count_guest(list_head));
				break;
			case 6:
				printf("Exiting...\n");
				return 0;
			default:
				printf("Invalid choice.\n");
		}
	}

	return 0;
}
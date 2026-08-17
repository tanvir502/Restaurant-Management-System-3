#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MENU 50
#define ADMIN_SECRET_KEY "admin123"



typedef struct {
    char method[20];
    char trx_id[30];
    char status[15];
} PaymentDetails;


typedef struct {
    char username[50];
    char password[50];
    char role[10];
} User;


typedef struct {
    int id;
    char name[50];
    double price;
} FoodItem;


typedef struct BSTNode {
    FoodItem item;
    struct BSTNode* left;
    struct BSTNode* right;
} BSTNode;


typedef struct OrderNode {
    int order_id;
    char customer_name[50];
    int food_id;
    char food_name[50];
    int quantity;
    double total_price;
    PaymentDetails payment;
    struct OrderNode* next;
} OrderNode;


typedef struct HistoryNode {
    int order_id;
    char customer_name[50];
    double total_price;
    PaymentDetails payment;
    struct HistoryNode* next;
    struct HistoryNode* prev;
} HistoryNode;


typedef struct QueueNode {
    OrderNode order_data;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* front;
    QueueNode* rear;
} KitchenQueue;


typedef struct StackNode {
    OrderNode canceled_order;
    struct StackNode* next;
} StackNode;



FoodItem menu[MAX_MENU];
int menu_count = 0;
BSTNode* bst_root = NULL;

OrderNode* order_head = NULL;
HistoryNode* history_head = NULL;
HistoryNode* history_tail = NULL;

KitchenQueue kitchen;
StackNode* undo_stack = NULL;

int global_order_id = 101;
User logged_in_user;
int is_logged_in = 0;


void clear_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


void load_menu_from_file();
void save_menu_to_file();
void save_order_to_file(OrderNode* order);

int register_user();
int login_user();
void logout_user();

void enqueue_kitchen(OrderNode order);
OrderNode dequeue_kitchen();

void push_undo(OrderNode order);
OrderNode pop_undo();

void remove_from_pending_list(int order_id);

void process_payment(PaymentDetails* payment, double amount);
void place_order();
void search_order();
void cancel_last_order();
void undo_cancel_order();
void view_bill();

void add_food_item();
void complete_order();
void view_history();
void view_all_payments();

void trigger_emergency_protocol();



BSTNode* insert_bst(BSTNode* root, FoodItem item) {
    if (root == NULL) {
        BSTNode* new_node = (BSTNode*)malloc(sizeof(BSTNode));
        new_node->item = item;
        new_node->left = new_node->right = NULL;
        return new_node;
    }
    if (item.id < root->item.id) {
        root->left = insert_bst(root->left, item);
    } else if (item.id > root->item.id) {
        root->right = insert_bst(root->right, item);
    }
    return root;
}

BSTNode* search_bst(BSTNode* root, int id) {
    if (root == NULL || root->item.id == id) {
        return root;
    }
    if (id < root->item.id) {
        return search_bst(root->left, id);
    }
    return search_bst(root->right, id);
}

void inorder_bst_display(BSTNode* root) {
    if (root != NULL) {
        inorder_bst_display(root->left);
        printf("%d\t%-15s\t%.2f BDT\n", root->item.id, root->item.name, root->item.price);
        inorder_bst_display(root->right);
    }
}

void search_food_in_bst() {
    int id;
    printf("\nEnter Food ID to Search in BST: ");
    if (scanf("%d", &id) != 1) {
        clear_buffer();
        printf("\nInvalid Input!\n");
        return;
    }
    clear_buffer();

    BSTNode* result = search_bst(bst_root, id);
    if (result != NULL) {
        printf("\n[BST SEARCH RESULT - FOUND]:\n");
        printf("Food ID: %d\nName: %s\nPrice: %.2f BDT\n",
               result->item.id, result->item.name, result->item.price);
    } else {
        printf("\n[BST SEARCH RESULT]: Food Item with ID %d not found!\n", id);
    }
}



void init_queue(KitchenQueue* q) {
    q->front = q->rear = NULL;
}

void enqueue_kitchen(OrderNode order) {
    QueueNode* new_node = (QueueNode*)malloc(sizeof(QueueNode));
    new_node->order_data = order;
    new_node->next = NULL;
    if (kitchen.rear == NULL) {
        kitchen.front = kitchen.rear = new_node;
        return;
    }
    kitchen.rear->next = new_node;
    kitchen.rear = new_node;
}

OrderNode dequeue_kitchen() {
    OrderNode empty_order = {0};
    if (kitchen.front == NULL) return empty_order;

    QueueNode* temp = kitchen.front;
    OrderNode data = temp->order_data;
    kitchen.front = kitchen.front->next;

    if (kitchen.front == NULL) kitchen.rear = NULL;
    free(temp);
    return data;
}

void push_undo(OrderNode order) {
    StackNode* new_node = (StackNode*)malloc(sizeof(StackNode));
    new_node->canceled_order = order;
    new_node->next = undo_stack;
    undo_stack = new_node;
}

OrderNode pop_undo() {
    OrderNode empty_order = {0};
    if (undo_stack == NULL) return empty_order;

    StackNode* temp = undo_stack;
    OrderNode data = temp->canceled_order;
    undo_stack = undo_stack->next;
    free(temp);
    return data;
}



int register_user() {
    User new_user;
    int role_choice;
    char secret_key[50];

    printf("\n--- REGISTER USER ---\n");
    printf("Enter Username: ");
    scanf(" %[^\n]", new_user.username);
    printf("Enter Password: ");
    scanf(" %[^\n]", new_user.password);

    printf("Select Role:\n");
    printf("1. CUSTOMER\n");
    printf("2. ADMIN\n");
    printf("Enter choice (1 or 2): ");
    scanf("%d", &role_choice);

    if (role_choice == 2) {
        printf("Enter Admin Passkey: ");
        scanf(" %[^\n]", secret_key);

        if (strcmp(secret_key, ADMIN_SECRET_KEY) != 0) {
            printf("Invalid Admin Passkey! Registration failed.\n");
            return 0;
        }
        strcpy(new_user.role, "ADMIN");
    } else {
        strcpy(new_user.role, "CUSTOMER");
    }

    FILE* file = fopen("users.txt", "a");
    if (file == NULL) {
        printf("Error accessing user database!\n");
        return 0;
    }

    fprintf(file, "%s %s %s\n", new_user.username, new_user.password, new_user.role);
    fclose(file);

    printf("Registration Successful as %s! You can now login.\n", new_user.role);
    return 1;
}

int login_user() {
    char input_username[50], input_password[50];
    User u;

    printf("\n--- LOGIN ---\n");
    printf("Enter Username: ");
    scanf(" %[^\n]", input_username);
    printf("Enter Password: ");
    scanf(" %[^\n]", input_password);

    FILE* file = fopen("users.txt", "r");
    if (file == NULL) {
        printf("No registered users found. Please register first.\n");
        return 0;
    }

    int found = 0;
    while (fscanf(file, "%s %s %s", u.username, u.password, u.role) != EOF) {
        if (strcmp(u.username, input_username) == 0 && strcmp(u.password, input_password) == 0) {
            logged_in_user = u;
            is_logged_in = 1;
            found = 1;
            break;
        }
    }
    fclose(file);

    if (found) {
        printf("Login Successful! Welcome, %s (%s)\n", logged_in_user.username, logged_in_user.role);
        return 1;
    } else {
        printf("Invalid Username or Password!\n");
        return 0;
    }
}

void logout_user() {
    is_logged_in = 0;
    memset(&logged_in_user, 0, sizeof(User));
    printf("Logged out successfully.\n");
}

void load_menu_from_file() {
    FILE* file = fopen("menu.txt", "r");
    if (file == NULL) {
        menu[0] = (FoodItem){1, "Burger", 180.0};
        menu[1] = (FoodItem){2, "Pizza", 450.0};
        menu[2] = (FoodItem){3, "Pasta", 250.0};
        menu_count = 3;

        for (int i = 0; i < menu_count; i++) {
            bst_root = insert_bst(bst_root, menu[i]);
        }

        save_menu_to_file();
        return;
    }
    menu_count = 0;
    while (fscanf(file, "%d,%49[^,],%lf\n", &menu[menu_count].id, menu[menu_count].name, &menu[menu_count].price) != EOF) {
        bst_root = insert_bst(bst_root, menu[menu_count]);
        menu_count++;
    }
    fclose(file);
}

void save_menu_to_file() {
    FILE* file = fopen("menu.txt", "w");
    if (file == NULL) return;
    for (int i = 0; i < menu_count; i++) {
        fprintf(file, "%d,%s,%.2f\n", menu[i].id, menu[i].name, menu[i].price);
    }
    fclose(file);
}

void save_order_to_file(OrderNode* order) {
    FILE* file = fopen("orders.txt", "a");
    if (file == NULL) return;
    fprintf(file, "ID: %d | Customer: %s | Food: %s | Qty: %d | Total: %.2f | Payment: %s | TrxID: %s | Status: %s\n",
            order->order_id, order->customer_name, order->food_name, order->quantity, order->total_price,
            order->payment.method, order->payment.trx_id, order->payment.status);
    fflush(file);
    fclose(file);
}



void remove_from_pending_list(int order_id) {
    OrderNode* temp = order_head;
    OrderNode* prev = NULL;

    if (temp != NULL && temp->order_id == order_id) {
        order_head = temp->next;
        free(temp);
        return;
    }

    while (temp != NULL && temp->order_id != order_id) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) return;

    prev->next = temp->next;
    free(temp);
}



void process_payment(PaymentDetails* payment, double amount) {
    int pay_choice;
    printf("\n========================================\n");
    printf("         PAYMENT GATEWAY (%.2f BDT)     \n", amount);
    printf("========================================\n");
    printf("1. Cash on Delivery\n");
    printf("2. bKash\n");
    printf("3. Nagad\n");
    printf("4. Credit / Debit Card\n");
    printf("Select Payment Gateway (1-4): ");

    if (scanf("%d", &pay_choice) != 1) {
        clear_buffer();
        pay_choice = 1;
    }
    clear_buffer();

    switch (pay_choice) {
        case 1:
            strcpy(payment->method, "Cash");
            strcpy(payment->trx_id, "N/A (Cash)");
            strcpy(payment->status, "PAID");
            printf("[PAYMENT SUCCESSFUL] Cash payment confirmed.\n");
            break;
        case 2:
            strcpy(payment->method, "bKash");
            printf("Enter bKash Transaction ID (TrxID): ");
            scanf(" %[^\n]", payment->trx_id);
            strcpy(payment->status, "PAID");
            printf("[PAYMENT SUCCESSFUL] bKash payment verified!\n");
            break;
        case 3:
            strcpy(payment->method, "Nagad");
            printf("Enter Nagad Transaction ID (TrxID): ");
            scanf(" %[^\n]", payment->trx_id);
            strcpy(payment->status, "PAID");
            printf("[PAYMENT SUCCESSFUL] Nagad payment verified!\n");
            break;
        case 4:
            strcpy(payment->method, "Card");
            printf("Enter Last 4 Digits of Card / Transaction Ref: ");
            scanf(" %[^\n]", payment->trx_id);
            strcpy(payment->status, "PAID");
            printf("[PAYMENT SUCCESSFUL] Card payment processed!\n");
            break;
        default:
            strcpy(payment->method, "Cash");
            strcpy(payment->trx_id, "N/A (Cash)");
            strcpy(payment->status, "PAID");
            printf("[DEFAULTED] Set to Cash on Delivery.\n");
            break;
    }
}



void display_menu() {
    printf("\n----------- FOOD MENU (Sorted using BST Inorder Traversal) -----------\n");
    printf("ID\tName\t\tPrice\n");
    printf("----------------------------------------------------------------------\n");
    inorder_bst_display(bst_root);
    printf("----------------------------------------------------------------------\n");
}

void place_order() {
    display_menu();
    OrderNode* new_order = (OrderNode*)malloc(sizeof(OrderNode));
    new_order->order_id = global_order_id++;

    strcpy(new_order->customer_name, logged_in_user.username);

    printf("Enter Food ID to Order: ");
    if (scanf("%d", &new_order->food_id) != 1) {
        clear_buffer();
        printf("Invalid Food ID!\n");
        free(new_order);
        return;
    }

    BSTNode* searched_item = search_bst(bst_root, new_order->food_id);

    if (searched_item == NULL) {
        printf("Invalid Food ID!\n");
        free(new_order);
        return;
    }

    strcpy(new_order->food_name, searched_item->item.name);
    printf("Enter Quantity: ");
    scanf("%d", &new_order->quantity);
    clear_buffer();

    new_order->total_price = searched_item->item.price * new_order->quantity;


    process_payment(&new_order->payment, new_order->total_price);

    new_order->next = order_head;
    order_head = new_order;

    enqueue_kitchen(*new_order);
    save_order_to_file(new_order);

    printf("\nOrder Placed Successfully! Your Order ID is: %d\n", new_order->order_id);
    printf("Payment Method: %s | Status: %s | Total: %.2f BDT\n",
           new_order->payment.method, new_order->payment.status, new_order->total_price);
}

void search_order() {
    int id;
    printf("Enter Order ID to Search: ");
    if (scanf("%d", &id) != 1) {
        clear_buffer();
        printf("Invalid Input!\n");
        return;
    }
    clear_buffer();

    OrderNode* temp = order_head;
    while (temp != NULL) {
        if (temp->order_id == id) {
            printf("\nOrder Found (Pending):\n");
            printf("Order ID    : %d\nCustomer    : %s\nFood        : %s (x%d)\nTotal       : %.2f BDT\nPayment     : %s (%s)\nTrxID       : %s\n",
                   temp->order_id, temp->customer_name, temp->food_name, temp->quantity, temp->total_price,
                   temp->payment.method, temp->payment.status, temp->payment.trx_id);
            return;
        }
        temp = temp->next;
    }
    printf("Order not found in Pending List (It might be completed or invalid).\n");
}

void cancel_last_order() {
    if (order_head == NULL) {
        printf("No active orders to cancel.\n");
        return;
    }

    OrderNode* temp = order_head;
    order_head = order_head->next;

    push_undo(*temp);

    printf("Order #%d canceled successfully and stored in Undo Stack.\n", temp->order_id);
    free(temp);
}

void undo_cancel_order() {
    OrderNode restored_order = pop_undo();
    if (restored_order.order_id == 0) {
        printf("No canceled orders available in Stack to undo!\n");
        return;
    }

    OrderNode* new_node = (OrderNode*)malloc(sizeof(OrderNode));
    *new_node = restored_order;

    new_node->next = order_head;
    order_head = new_node;

    enqueue_kitchen(*new_node);

    printf("\nSUCCESS: Restored Canceled Order #%d for %s from Stack back to Pending Orders!\n",
           new_node->order_id, new_node->customer_name);
}

void view_bill() {
    int id;
    printf("Enter Order ID for Bill: ");
    if (scanf("%d", &id) != 1) {
        clear_buffer();
        printf("Invalid Input!\n");
        return;
    }
    clear_buffer();

    OrderNode* temp = order_head;
    while (temp != NULL) {
        if (temp->order_id == id) {
            printf("\n================ OFFICIAL INVOICE ================\n");
            printf(" Order ID        : %d\n", temp->order_id);
            printf(" Customer        : %s\n", temp->customer_name);
            printf(" Item            : %s (x%d)\n", temp->food_name, temp->quantity);
            printf(" Total Price     : %.2f BDT\n", temp->total_price);
            printf(" Payment Method  : %s\n", temp->payment.method);
            printf(" Payment Status  : %s\n", temp->payment.status);
            printf(" Transaction Ref : %s\n", temp->payment.trx_id);
            printf("==================================================\n");
            return;
        }
        temp = temp->next;
    }
    printf("Order ID not found in pending list.\n");
}



void add_food_item() {
    if (menu_count >= MAX_MENU) {
        printf("Menu is full!\n");
        return;
    }
    FoodItem item;
    item.id = menu_count + 1;
    printf("Enter Food Name: ");
    scanf(" %[^\n]", item.name);
    printf("Enter Price: ");
    scanf("%lf", &item.price);
    clear_buffer();

    menu[menu_count++] = item;
    bst_root = insert_bst(bst_root, item);
    save_menu_to_file();
    printf("New Food Item Added Successfully and Inserted into BST!\n");
}

void complete_order() {
    OrderNode kitchen_order = dequeue_kitchen();
    if (kitchen_order.order_id == 0) {
        printf("No orders in kitchen queue.\n");
        return;
    }

    HistoryNode* hnode = (HistoryNode*)malloc(sizeof(HistoryNode));
    hnode->order_id = kitchen_order.order_id;
    strcpy(hnode->customer_name, kitchen_order.customer_name);
    hnode->total_price = kitchen_order.total_price;
    hnode->payment = kitchen_order.payment;
    hnode->next = NULL;
    hnode->prev = history_tail;

    if (history_tail != NULL) history_tail->next = hnode;
    history_tail = hnode;
    if (history_head == NULL) history_head = hnode;

    remove_from_pending_list(kitchen_order.order_id);

    printf("Order #%d for %s prepared by Kitchen, removed from pending list, and marked COMPLETE!\n",
           kitchen_order.order_id, kitchen_order.customer_name);
}

void view_history() {
    printf("\n----------- COMPLETED ORDER HISTORY (Doubly Linked List) -----------\n");
    HistoryNode* temp = history_head;
    if (temp == NULL) {
        printf("No history available.\n");
        return;
    }
    while (temp != NULL) {
        printf("Order ID: %d | Customer: %s | Total: %.2f BDT | Payment: %s (%s) | TrxID: %s\n",
               temp->order_id, temp->customer_name, temp->total_price,
               temp->payment.method, temp->payment.status, temp->payment.trx_id);
        temp = temp->next;
    }
    printf("--------------------------------------------------------------------\n");
}

void view_all_payments() {
    printf("\n================ SYSTEM PAYMENT AUDIT LOG ================\n");
    printf("ID\tCustomer\tAmount\t\tMethod\tStatus\tTrxID\n");
    printf("----------------------------------------------------------\n");

    OrderNode* temp_p = order_head;
    while (temp_p != NULL) {
        printf("%d\t%-10s\t%.2f BDT\t%s\t%s\t%s\n",
               temp_p->order_id, temp_p->customer_name, temp_p->total_price,
               temp_p->payment.method, temp_p->payment.status, temp_p->payment.trx_id);
        temp_p = temp_p->next;
    }

    HistoryNode* temp_h = history_head;
    while (temp_h != NULL) {
        printf("%d\t%-10s\t%.2f BDT\t%s\t%s\t%s\n",
               temp_h->order_id, temp_h->customer_name, temp_h->total_price,
               temp_h->payment.method, temp_h->payment.status, temp_h->payment.trx_id);
        temp_h = temp_h->next;
    }
    printf("==========================================================\n");
}



void trigger_emergency_protocol() {

    printf("===============================================================\n");
    printf("          !!! EMERGENCY / EARTHQUAKE WARNING !!!               \n");
    printf("  PLEASE EVACUATE THE BUILDING IMMEDIATELY VIA EMERGENCY EXITS  \n");
    printf("===============================================================\n");


    FILE* fp = fopen("emergency_snapshot.txt", "w");
    if (fp != NULL) {
        fprintf(fp, "=== EMERGENCY STATE DUMP ===\n");
        OrderNode* temp = order_head;
        while (temp != NULL) {
            fprintf(fp, "PENDING ORDER ID: %d | Customer: %s | Amount: %.2f | Payment: %s | TrxID: %s\n",
                    temp->order_id, temp->customer_name, temp->total_price, temp->payment.method, temp->payment.trx_id);
            temp = temp->next;
        }
        fflush(fp);
        fclose(fp);
        printf("[SYSTEM] Emergency snapshot saved to 'emergency_snapshot.txt'.\n");
    }

    printf("[SYSTEM] System safely locked down.\n");
    exit(0);
}



void customer_panel() {
    int choice;
    while (is_logged_in) {
        printf("\n--- CUSTOMER PANEL (%s) ---\n", logged_in_user.username);
        printf("1. View Menu (BST Inorder Traversal)\n");
        printf("2. Place Order & Pay (Payment Gateway Integrated)\n");
        printf("3. Search Food Item (BST Search)\n");
        printf("4. Search Order Status\n");
        printf("5. View Invoice/Bill\n");
        printf("6. Cancel Order (Push to Stack)\n");
        printf("7. Undo Canceled Order (Pop from Stack)\n");
        printf("8. Logout\n");
        printf("Enter Choice: ");

        if (scanf("%d", &choice) != 1) {
            clear_buffer();
            printf("Invalid Choice! Please enter a number.\n");
            continue;
        }
        clear_buffer();

        switch (choice) {
            case 1: display_menu(); break;
            case 2: place_order(); break;
            case 3: search_food_in_bst(); break;
            case 4: search_order(); break;
            case 5: view_bill(); break;
            case 6: cancel_last_order(); break;
            case 7: undo_cancel_order(); break;
            case 8: logout_user(); return;
            default: printf("Invalid Choice!\n");
        }
    }
}

void admin_panel() {
    int choice;
    while (is_logged_in) {
        printf("\n--- ADMIN PANEL (%s) ---\n", logged_in_user.username);
        printf("1. Add Food Item (Insert to BST)\n");
        printf("2. Complete Kitchen Order (Queue Dequeue & List Remove)\n");
        printf("3. View Order History (Doubly Linked List)\n");
        printf("4. View All Payments/Transactions Audit\n");
        printf("5. Logout\n");
        printf("Enter Choice: ");

        if (scanf("%d", &choice) != 1) {
            clear_buffer();
            printf("Invalid Choice! Please enter a number.\n");
            continue;
        }
        clear_buffer();

        switch (choice) {
            case 1: add_food_item(); break;
            case 2: complete_order(); break;
            case 3: view_history(); break;
            case 4: view_all_payments(); break;
            case 5: logout_user(); return;
            default: printf("Invalid Choice!\n");
        }
    }
}

int main() {
    init_queue(&kitchen);
    load_menu_from_file();

    int choice;
    while (1) {
        printf("\n=========================================\n");
        printf("    RESTAURANT ORDER MANAGEMENT SYSTEM    \n");
        printf("=========================================\n");
        printf("1. Register (Admin / Customer)\n");
        printf("2. Login\n");
        printf("3. EMERGENCY EXIT (Earthquake / Disaster)\n");
        printf("4. Exit System\n");
        printf("Enter Choice: ");

        if (scanf("%d", &choice) != 1) {
            clear_buffer();
            printf("Invalid Choice! Please enter a number.\n");
            continue;
        }
        clear_buffer();

        switch (choice) {
            case 1:
                register_user();
                break;
            case 2:
                if (login_user()) {
                    if (strcmp(logged_in_user.role, "ADMIN") == 0) {
                        admin_panel();
                    } else {
                        customer_panel();
                    }
                }
                break;
            case 3:
                trigger_emergency_protocol();
                break;
            case 4:
                exit(0);
            default:
                printf("Invalid Choice!\n");
        }
    }
    return 0;
}



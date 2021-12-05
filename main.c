#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define MAX_CITIES (100000 + 1)          // One city for the airport
#define MAX_ROUTES (100000 + MAX_CITIES) // All routes, plus one route between each city and the airport.

#define DEFAULT_CAPACITY 128
#define IMPOSSIBLE -1

/**
 * A data structure which contains information about the current graph of cities. This data structure can then be
 * easily allocated on the stack, since it has a fixed size.
 */
typedef struct graph {

  /** The number of cities of the graph. */
  size_t size;

  /** The number of cities which are reachable from the provided city. */
  int degrees[MAX_CITIES + 1];

  /** The offset in the neighbours adjacency list where the neighbours of the i-th city start. */
  int start[MAX_CITIES + 1];

  /** The neighbours of the city at the provided index. Each edge goes in two directions. */
  int neighbours[2 * MAX_ROUTES];
} graph_t;

/**
 * A data structure which represents at edge between two nodes, starting at from and ending at to.
 */
typedef struct edge {
  int from, to;
} edge_t;

/**
 * A dynamic circular buffer, which contains some items and may be iterated in a circular fashion. The buffer has a
 * given capacity, which will increase if it's full. When increased, the items from the buffer will be copied into a
 * new circular buffer.
 */
typedef struct circular_buffer {

  /** The number of items that may be present in the buffer at once. */
  size_t capacity;

  /** The index of the first item. */
  size_t start;

  /** How many items are in the buffer right now. */
  size_t size;

  /** A dynamically allocated array of the circular buffer elements. */
  int *elements;
} circular_buffer_t;

/**
 * Creates a new circular buffer, which the provided capacity and no inner items.
 * @param capacity the capacity of the buffer. Must be strictly positive.
 * @return the pointer to the newly allocated buffer. NULL if an error occurred.
 */
circular_buffer_t *make_circular_buffer(size_t capacity) {
  if (capacity == 0) return NULL;
  circular_buffer_t *ptr = (circular_buffer_t *) malloc(sizeof(circular_buffer_t));
  int *elements = (int *) calloc(capacity, sizeof(int));
  if (!ptr || !elements) {
    free(ptr);
    free(elements);
    return NULL;
  }
  ptr->capacity = capacity;
  ptr->size = 0;
  ptr->start = 0;
  ptr->elements = elements;
  return ptr;
}

/**
 * Enqueues an item at the tail of the circular buffer. This runs in O(n) amortized time, since the buffer may require
 * to grow to accommodate for the newly inserted item.
 * @param buffer the circular buffer to which an item is added.
 * @param element the enqueued element.
 * @return 0, or 1 if an error occurred.
 */
int circular_buffer_enqueue(circular_buffer_t *buffer, int element) {
  if (buffer->capacity == buffer->size) {
    int *space = (int *) calloc(buffer->capacity * 2, sizeof(int));
    if (!space) return 1; // We could not increase the buffer capacity.

    // TODO: A bit ugly, but essentially, we can simply duplicate the contents of the buffer rather than calculate good bounds.
    memcpy(space, buffer->elements, buffer->capacity * sizeof(int));
    memcpy(&space[buffer->capacity], buffer->elements, buffer->capacity * sizeof(int));

    // Update the buffer structure.
    buffer->capacity *= 2;
    free(buffer->elements);
    buffer->elements = space;
  }
  size_t index = (buffer->start + buffer->size) % buffer->capacity;
  buffer->elements[index] = element;
  buffer->size++;
  return 0;
}

/**
 * Dequeues the head item from this circular buffer. This will fetch the item, move the start index, and finally
 * decrease the size of the buffer to account for the removal.
 * @param buffer the buffer from which the element is removed.
 * @return the dequeued element.
 */
int circular_buffer_dequeue(circular_buffer_t *buffer) {
  if (buffer->size == 0) raise(SIGSEGV); // We do not expect callers to make this call. This is a bad violation.
  size_t index = buffer->start % buffer->capacity;
  int item = buffer->elements[index];
  buffer->size--;
  buffer->start = (buffer->start + 1) % buffer->capacity;
  return item;
}

/**
 * The graph in which the model will be stored.
 */
graph_t graph;

int solve(int from, int until) {
  circular_buffer_t *queue = make_circular_buffer(DEFAULT_CAPACITY);
  if (!queue) return IMPOSSIBLE;
  int distance = 1;
  bool visited[graph.size];
  memset(visited, 0, graph.size * sizeof(bool));

  circular_buffer_enqueue(queue, from);
  while (queue->size > 0) {
    int head = circular_buffer_dequeue(queue);
    if (head < 0) {
      distance = -head;
    } else if (head == until) {
      return distance - 1;
    } else {
      if (graph.degrees[head] > 0) circular_buffer_enqueue(queue, -distance - 1);
      for (int i = 0; i < graph.degrees[head]; i++) {
        int city = graph.neighbours[graph.start[head] + i];
        if (!visited[city]) {
          circular_buffer_enqueue(queue, city);
          visited[city] = true;
        }
      }
    }
  }
  return IMPOSSIBLE;
}

#define BUFFER_SIZE (16 * 4096)

// A buffer large enough to store any line we're given.
char input_buffer[BUFFER_SIZE];
char *input_ptr = input_buffer;
char *input_ptr_end = input_buffer + BUFFER_SIZE - 1;

/**
 * Initialize the scanner with some proper values.
 */
void scan_init() {
  input_buffer[BUFFER_SIZE - 1] = '\0'; // Null-terminate the input buffer.
  fread(input_buffer, sizeof(char), BUFFER_SIZE - 1, stdin);
  input_ptr = input_buffer;
}

/** Parses the next multi-digit integer. */
int scan_int() {
  int n = 0;
  while (*input_ptr < '0' || *input_ptr > '9') {
    ++input_ptr;
    if (input_ptr == input_ptr_end) {
      size_t read = fread(input_buffer, sizeof(char), BUFFER_SIZE - 1, stdin);
      if (read == 0) input_buffer[0] = '\0';
      input_ptr = input_buffer;
    }
  }
  while (*input_ptr >= '0' && *input_ptr <= '9') {
    n *= 10;
    n += *input_ptr - '0';
    ++input_ptr;
    if (input_ptr == input_ptr_end) {
      size_t read = fread(input_buffer, sizeof(char), BUFFER_SIZE - 1, stdin);
      if (read == 0) input_buffer[0] = '\0';
      input_ptr = input_buffer;
    }
  }
  return n;
}

int main() {

  scan_init();

  int n = scan_int();
  int m = scan_int();
  int k = scan_int();
  int s = scan_int();
  int t = scan_int();

  int airports[k];
  edge_t edges[m];
  graph.size = n + 1;

  for (int i = 0; i < k; i++) {
    int city = scan_int();
    airports[i] = city;
    graph.degrees[0]++;
    graph.degrees[city]++;
  }
  for (int i = 0; i < m; i++) {
    int a = scan_int();
    int b = scan_int();
    edges[i].from = a;
    edges[i].to = b;
    graph.degrees[a]++;
    graph.degrees[b]++;
  }

  // We can now compute the offsets.
  int start = 0;
  for (int i = 0; i < n + 2; i++) {
    graph.start[i] = start;
    start += graph.degrees[i];
    graph.degrees[i] = 0; // Reset the degrees, so we can use them afterwards when we're adding items.
  }

  // Finally, add the proper normal edges.
  for (int i = 0; i < m; i++) {
    edge_t edge = edges[i];
    size_t from_index = graph.start[edge.from] + graph.degrees[edge.from];
    size_t to_index = graph.start[edge.to] + graph.degrees[edge.to];
    graph.neighbours[from_index] = edge.to;
    graph.neighbours[to_index] = edge.from;
    graph.degrees[edge.from]++;
    graph.degrees[edge.to]++;
  }
  // And the airports.
  for (int i = 0; i < k; i++) {
    int airport = airports[i];
    size_t from_index = graph.start[0] + graph.degrees[0];
    size_t to_index = graph.start[airport] + graph.degrees[airport];
    graph.neighbours[from_index] = airport;
    graph.neighbours[to_index] = 0;
    graph.degrees[0]++;
    graph.degrees[airport]++;
  }

  int result = solve(s, t);
  if (result == IMPOSSIBLE) {
    printf("Impossible\n");
  } else {
    printf("%d\n", result);
  }

  return 0;
}

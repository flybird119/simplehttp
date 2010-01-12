
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "event.h"
#include "evhttp.h"
#include "pubsub_to_pubsub.h"
#define DEBUG 0


struct global_data {
    int (*cb)(char *data, struct evbuffer *evb, char **target_path, void *cbarg);
    struct evhttp_connection *evhttp_target_connection;
    char *target_address;
    char *target_path;
    void *cbarg;
};

void 
http_post_done(struct evhttp_request *req, void *arg){
    if (req->response_code != HTTP_OK) {
        fprintf(stderr, "FAILED post != OK\n");
        exit(1);
    }
    if (DEBUG) fprintf(stdout, "post_done OK\n");
}


void 
source_callback (struct evhttp_request *req, void *arg){
    if (DEBUG) fprintf(stdout, "source_callback\n");
    // enum message_read_status done;
    evhttp_clear_headers(req->input_headers);
    evhttp_parse_headers(req, req->input_buffer);
    char *content_len;
    content_len = (char *) evhttp_find_header(req->input_headers, "Content-Length");
    if (!content_len){return;}
    
    int len = atoi(content_len);
    size_t len_size;
    
    len_size = (size_t)len;
    if (DEBUG) fprintf(stdout, "received content_length:%d buffer has:%d\n", len, EVBUFFER_LENGTH(req->input_buffer));

    struct global_data *client_data = (struct global_data *)arg;

    struct evhttp_request *evhttp_target_request = NULL;

    evhttp_target_request = evhttp_request_new(http_post_done, NULL);
    evhttp_add_header(evhttp_target_request->output_headers, "Host", client_data->target_address);

    char *data = calloc(len, sizeof(char *));
    evbuffer_remove(req->input_buffer, data, len);
    if (DEBUG)fprintf(stdout, "data has %d bytes\n", strlen(data));
    if (DEBUG)fprintf(stdout, "data=%s\n", data);
    // empty buffer
    evbuffer_drain(req->input_buffer, EVBUFFER_LENGTH(req->input_buffer));
    // write to output buffer
    int flag = (*client_data->cb)(data, evhttp_target_request->output_buffer, &client_data->target_path, client_data->cbarg);
    free(data);
    if (!flag){return;} // don't make the request

    if (evhttp_make_request(client_data->evhttp_target_connection, evhttp_target_request, EVHTTP_REQ_POST, client_data->target_path) == -1) {
        fprintf(stdout, "FAILED make_request\n");
        exit(1);
    }
}

void
http_chunked_request_done(struct evhttp_request *req, void *arg)
{
    if (DEBUG) fprintf(stdout, "http_chunked_request_done\n");
    if (req->response_code != HTTP_OK) {
        fprintf(stderr, "FAILED on sub connection\n");
        exit(1);
    }
    if (DEBUG) fprintf(stderr, "DONE on sub connection\n");
}

int
pubsub_to_pubsub_main(char *source_address, int source_port, char *target_address, int target_port, int (*cb)(char *data, struct evbuffer *evb, char **target_path, void *arg), void *cbarg){
    return pubsub_to_pubsub_main_path(source_address, source_port, target_address, target_port, cb, cbarg, "/pub");
}

int
pubsub_to_pubsub_main_path(char *source_address, int source_port, char *target_address, int target_port, int (*cb)(char *data, struct evbuffer *evb, char **target_path, void *arg), void *cbarg, char *target_path)
{

    event_init();

    struct evhttp_connection *evhttp_source_connection = NULL;
    struct evhttp_connection *evhttp_target_connection = NULL;
    struct evhttp_request *evhttp_source_request = NULL;
    
    fprintf(stdout, "connecting to http://%s:%d/sub\n", source_address, source_port);
    evhttp_source_connection = evhttp_connection_new(source_address, source_port);
    if (evhttp_source_connection == NULL) {
        fprintf(stdout, "FAILED CONNECT TO SOURCE %s:%d\n", source_address, source_port);
        exit(1);
    }

    fprintf(stdout, "connecting to http://%s:%d/pub\n", target_address, target_port);
    evhttp_target_connection = evhttp_connection_new(target_address, target_port);
    if (evhttp_target_connection == NULL) {
        fprintf(stdout, "FAILED CONNECT TO TARGET %s:%d\n", target_address, target_port);
        exit(1);
    }

    struct global_data *data;
    data = calloc(1, sizeof(*data));
    data->evhttp_target_connection = evhttp_target_connection;
    data->target_address = target_address;
    data->target_path = calloc(4096, sizeof(char *));
    strcpy(data->target_path, target_path);
    data->cb = cb;
    data->cbarg = cbarg;

    evhttp_source_request = evhttp_request_new(http_chunked_request_done, data);
    evhttp_add_header(evhttp_source_request->output_headers, "Host", source_address);
    evhttp_request_set_chunked_cb(evhttp_source_request, source_callback);

    if (evhttp_make_request(evhttp_source_connection, evhttp_source_request, EVHTTP_REQ_GET, "/sub") == -1) {
        fprintf(stdout, "FAILED make_request to source\n");
        exit(1);
    }

    event_dispatch();
    evhttp_connection_free(evhttp_source_connection);
    evhttp_connection_free(evhttp_target_connection);
    fprintf(stdout, "EXITING\n");

    return 0;
}

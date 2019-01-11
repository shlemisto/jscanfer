# jscanfer

This code is just a part of big project... It just shows a way how to implement generic-like json parser

An example of how it may be used:

```
  // json: { ..., "params": {"service": "name", "node_id": 2, "state": 1, "type": 2, "update": false, "node_info": { ... } }, ...}

  typedef struct xxx_req {
      char *service;
      uds_node_id_t node_id;
      unsigned int state;
      uds_type_t type;
      bool update;
      cJSON *node_info;
  } xxx_req_t;

  ...

  xxx_req_t *req = uds_alloc(xxx_req_t);
  
  ...

   if (json_parse(p->params_obj, "destination", &req->service) ||
       json_parse(p->params_obj, "node_id", &req->node_id) ||
       json_parse(p->params_obj, "state", &req->state) ||
       json_parse(p->params_obj, "type", &req->type) ||
       json_parse(p->params_obj, "active", &req->active) ||
       json_parse_with_default(p->params_obj, "update", &req->update, true) || // assign 'true' if field 'update' not found
       json_parse_with_default(p->params_obj, "node_info", &req->node_info, cJSON_CreateObject()))
   {
       err("parsing of method '%s' finished with error: '%s'", method_map->method_name, g_json_parse_err_buf);
       goto exit;
   }
   
   ...
   
   // free memory
```

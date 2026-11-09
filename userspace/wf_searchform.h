/* wf_searchform.h - Lot 85: decide what a submitted HTML search form
 * should do. GET and POST already build an application/x-www-form-urlencoded
 * request (wf_form_submit). This looks at the URL and the body and says
 * whether that request is a search the guest can answer without the
 * site's JavaScript.
 *
 *   WF_ROUTE_FETCH  fetch the URL (and POST body) as built
 *   WF_ROUTE_GOOGLE the query is a Google search. The HTML page asks
 *                   for JavaScript; the guest uses the no-JS path.
 *   WF_ROUTE_WIKI   the query is a Wikipedia search. The HTML form posts
 *                   to index.php; the guest uses the Wikipedia API.
 *
 * A docs site, a news site, or any other host stays WF_ROUTE_FETCH:
 * the GET query or the POST body is the real submit. */
#ifndef WF_SEARCHFORM_H
#define WF_SEARCHFORM_H

#define WF_ROUTE_FETCH  0
#define WF_ROUTE_GOOGLE 1
#define WF_ROUTE_WIKI   2

/* url is the absolute submit URL (GET already has the query). body is
 * the POST body, or 0 for GET. On a search route, query receives the
 * decoded words (q, query, search, or s). Returns a WF_ROUTE_* value.
 * An empty query is WF_ROUTE_FETCH. */
int wf_search_route(const char *url, const char *body, char *query, int cap);

/* 1 when this control name is a search box (q, query, search, s). */
int wf_search_field_name(const char *name);
#endif

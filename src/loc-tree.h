#ifndef LOC_TREE_H
#define LOC_TREE_H 1

/** Tree column definitions */
typedef enum {
    TREE_COL_NAM = 0,           /*!< Location name column. */
    TREE_COL_LAT,               /*!< Location latitude column. */
    TREE_COL_LON,               /*!< Location longitude column. */
    TREE_COL_ALT,               /*!< Location altitude column. */
    TREE_COL_WX,                /*!< Weather station column. */
    TREE_COL_SELECT,            /*!< Invisible colindicating whether row may be selected */
    TREE_COL_NUM                /*!< The total number of columns. */
} loc_tree_col_t;


/** Column flags */
typedef enum {
    TREE_COL_FLAG_NAME = 1 << TREE_COL_NAM,     /*!< Location name column. */
    TREE_COL_FLAG_LAT = 1 << TREE_COL_LAT,      /*!< Location latitude column. */
    TREE_COL_FLAG_LON = 1 << TREE_COL_LON,      /*!< Location longitude column. */
    TREE_COL_FLAG_ALT = 1 << TREE_COL_ALT,      /*!< Location altitude column. */
    TREE_COL_FLAG_WX = 1 << TREE_COL_WX /*!< Weather station column. */
} loc_tree_col_flag_t;



gboolean        loc_tree_create(const gchar * fname,
                                guint flags,
                                gchar ** loc,
                                gfloat * lat,
                                gfloat * lon, guint * alt, gchar ** wx);

#endif

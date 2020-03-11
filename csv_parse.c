#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

typedef struct	data_s
{
	struct data_s	*next;
	char			**val;
}
data_t;

void	llist_push(void *l, void *elem)
{
	register char	**list = (char **)l;

	if (*list == NULL)
	{
		*list = elem;
		return ;
	}

	register char	*iter = *list;
	while (*(char **)iter)
		iter = *(char **)iter;
	*(char **)iter = (char *)elem;
}



char	*get_file_content(const char *fn)
{
	int			fd = open(fn, O_RDONLY);
	char		*content = NULL;
	uint32_t	size;

	if (fd == -1)
		return (NULL);
	
	size = lseek(fd, 0, SEEK_END);
	if (size == 0)
	{
		content = malloc(1);
		*content = '\0';
		return (content);
	}

	lseek(fd, 0, SEEK_SET);
	if ((content = malloc(size + 1)) == NULL)
		return (content);

	if (read(fd, content, size) != size)
	{
		free(content);
		return (NULL);
	}
	content[size] = 0;

	close(fd);
	
	return (content);
}


uint32_t	max(const uint32_t *t, uint32_t size)
{
	uint32_t	max = 0;

	for (uint32_t i = 0; i != size; i++)
	{
		if (t[i] > max)
			max = t[i];
	}

	return (max + 1);
}

void	parse(data_t **data, char *s, const char del, const uint32_t *fields, uint32_t skip, uint32_t nfields)
{
	data_t			*new = NULL;
	uint32_t		total_fields = max(fields, nfields);

	if (nfields == 0)
	{
		fprintf(stderr, "ERROR: No field to print\n");
		exit(1);
	}
	*data = NULL;
	while (skip)
	{
		while (*s && *s != '\n') s++;
		if (*s == '\0')
		{
			fprintf(stderr, "ERROR: No data to parse\n");
			exit(1);
		}
		s++;
		skip--;
	}

	for (uint32_t i = 0, cur_field = 0, new_index = 0; ; i++)
	{
		while (*s && *s == del) s++;
		if (!*s)
			goto __end;

		if (i == 0)
		{
			if (new)
				llist_push(data, new);
			new = malloc(sizeof(data_t));
			new->next = NULL;
			new->val = malloc(sizeof(char*) * nfields);
		}

		if (i == total_fields)
		{
			while (*s && *s != '\n') s++;
			if (!*s)
				goto __end;
			s++;
			new_index = 0;
			cur_field = 0;
			i = 0xFFFFFFFFu;
			continue;
		}
		else if (fields[cur_field] != i)
		{
			while (*s && *s != del && *s != '\n') s++;
			if (*s == '\n')
			{
				new_index = 0;
				cur_field = 0;
				i = 0xFFFFFFFFu;
				s++;
			}
			continue;
		}
		printf("else\n");

		new->val[new_index++] = s;
		while (*s && *s != del && *s != '\n') s++;
		if (*s)
		{
			if (*s == '\n')
			{
				new_index = 0;
				cur_field = 0;
				i = 0xFFFFFFFFu;
			}
			else
				cur_field++;
			*(s++) = 0;
		}
	}


	__end:
	if (new)
		llist_push(data, new);
}

void	print_req(data_t *data, const uint32_t *order, char **fmt, uint32_t n_fields)
{
	if (fmt)
	{
		while (data)
		{
			dprintf(STDOUT_FILENO, "%s%s", fmt[0], data->val[order[0]]);
			for (uint32_t i = 1; i != n_fields; i++)
				dprintf(STDOUT_FILENO, "%s%s",  fmt[i], data->val[order[i]]);
			if (fmt[n_fields])
				dprintf(STDOUT_FILENO, "%s", fmt[n_fields]);
			data = data->next;
		}
	}
	else
	{
		while (data)
		{
			printf("%s", data->val[order[0]]);
			for (uint32_t i = 1; i != n_fields; i++)
				printf(" %s", data->val[order[i]]);
			printf("\n");
			data = data->next;
		}
	}
}

uint32_t	count_fmt(const char *fmt)
{
	const char	*start = fmt;
	uint32_t	count = 0;

	while (*fmt)
	{
		if (*fmt == '{' && (fmt == start || fmt[-1] != '\\'))
			count++;
		fmt++;
	}

	return (count);
}

uint8_t	uniq(const uint32_t *t, uint32_t size)
{
	for (uint32_t i = 0; i != size; i++)
	{
		for (uint32_t j = i + 1; j != size; j++)
		{
			if (t[i] == t[j])
				return (0);
		}
	}
	return (1);
}

char	**check_fmt(char *fmt, uint32_t **order, uint32_t *nfields)
{
	char		*start = fmt;
	uint32_t	tmp;
	char		**format = NULL;
	uint32_t	cur_order = 0;

	*nfields = count_fmt(fmt);
	*order = malloc(*nfields * sizeof(uint32_t));
	format = malloc((*nfields + 1) * sizeof(char *));

	while (*fmt)
	{
		if (*fmt == '\\')
		{
			if (fmt[1] == 'n')
				fmt[0] = '\n';
			else
				fmt[0] = fmt[1];
			fmt++;
			memmove(fmt, fmt + 1, strlen(fmt));
		}
		else if (*fmt == '{')
		{
			format[cur_order] = start;
			*(fmt++) = '\0';
			if (*fmt < '0' || *fmt > '9')
			{
				fprintf(stderr, "ERROR: syntax in [fmt]: {[0-9]+}\n");
				exit(1);
			}
			tmp = *(fmt++) - '0';
			while (*fmt >= '0' && *fmt <= '9')
				tmp = tmp * 10 + *(fmt++) - '0';
			if (*fmt != '}')
			{
				fprintf(stderr, "ERROR: missing '}' in [fmt]\n");
				exit(1);
			}
			if (tmp > 0xFFFFu)
			{
				fprintf(stderr, "ERROR: max field is 65535\n");
				exit(1);
			}
			(*order)[cur_order++] = tmp;
			fmt++;
			start = fmt;
		}
		else
			fmt++;
	}

	if (start != fmt)
		format[cur_order] = start;
	else
		format[cur_order] = NULL;

	if (!uniq(*order, *nfields))
	{
		fprintf(stderr, "ERROR: duplicate index in [fmt]\n");
		exit(1);
	}

	return (format);
}

void		buble_sort(uint32_t *t, uint32_t size)
{
	uint32_t		mem;
	uint32_t		tri_ok;

	do
	{
		tri_ok = 1;
		for (uint32_t i = 0; i != size - 1; i++)
		{
			if (t[i] > t[i+1])
			{
				mem = t[i];
				t[i] = t[i+1];
				t[i+1] = mem;
				tri_ok = 0;
			}
		}
	}
	while (!tri_ok);
}

uint32_t		*get_fields(uint32_t *order, uint32_t n_fields)
{
	uint32_t	*sorted = malloc(n_fields * sizeof(uint32_t));

	memcpy(sorted, order, n_fields * sizeof(uint32_t));
	buble_sort(sorted, n_fields);

	return (sorted);
}

void		reorganize_index(uint32_t *t, uint32_t size)
{
	uint32_t	maxfield = max(t, size);
	uint32_t	occ[0xFFFFu] = {0};
	uint32_t	hole_start = 0xFFFFu;
	uint32_t	dec;
	uint32_t	off = 0;

/*
	printf("REORGANIZE_INDEX mae = %u", *t);
	for (uint32_t i = 1; i != size; i++)
		printf(" %u", t[i]);
	printf("\n");
*/

	for (uint32_t i = 0; i != size; i++)
		occ[t[i]]++;

	for (uint32_t i = 0; i != maxfield; i++)
	{
		if (occ[i] == 0)
		{
			hole_start = i;
			++i;
			while (i != maxfield && occ[i] == 0) i++;
			dec = i - hole_start;
			for (uint32_t j = 0; j != size; j++)
			{
				if (t[j] > hole_start - off)
					t[j] -= dec;
			}
			off++;
		}
	}

/*
	printf("REORGANIZE_INDEX ato = %u", *t);
	for (uint32_t i = 1; i != size; i++)
		printf(" %u", t[i]);
	printf("\n");
*/
}


int		main(int argc, char **argv)
{
	char		*filename;
	char		*content;
	uint32_t	*order;
	data_t		*data = NULL;
	uint32_t	skip = 0;
	char		del;
	char		*fmt;
	char		**format = NULL;
	uint32_t	nfields = 0;
	uint32_t	*fields;

	if (argc != 4 && argc != 5)
	{
		printf(
			"%s [file.csv] [del] [fmt] [skip]?\n\n"
			"del    : one character delimiter\n"
			"fmt    : formated string for printing ({0} = field 0, \\{ = {)\n"
			"skip   : number of lines to skip at start of file (default = 0)\n\n",
			*argv
		);
		return (1);
	}

	filename = argv[1];
	del = *argv[2];
	fmt = argv[3];
	if (argc == 5)
		sscanf(argv[argc - 1], "%u", &skip);

	if (del == '\0')
	{
		fprintf(stderr, "ERROR: delemiter can't be a nullbyte\n");
		return (1);
	}
	format = check_fmt(fmt, &order, &nfields);
	fields = get_fields(order, nfields);
	reorganize_index(order, nfields);
	content = get_file_content(filename);
	parse(&data, content, del, fields, skip, nfields);
	print_req(data, order, format, nfields);

	return (0);
}

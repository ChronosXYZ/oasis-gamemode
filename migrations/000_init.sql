-- public.users definition

-- Drop table

-- DROP TABLE public.users;

CREATE TABLE public.users (
	id serial4 NOT NULL,
	"name" text NOT NULL,
	password_hash text NOT NULL,
	"language" text NOT NULL DEFAULT 'en'::text,
	email text NOT NULL,
	last_skin_id int2 NULL,
	last_ip text NULL,
	last_login_at timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
	registration_date timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
	CONSTRAINT users_pk PRIMARY KEY (id),
	CONSTRAINT users_unique UNIQUE (name, email)
);


-- public.admins definition

-- Drop table

-- DROP TABLE public.admins;

CREATE TABLE public.admins (
	user_id serial4 NOT NULL,
	"level" int2 NOT NULL DEFAULT 0,
	password_hash text NULL,
	CONSTRAINT admins_users_fk FOREIGN KEY (user_id) REFERENCES public.users(id) ON DELETE CASCADE ON UPDATE CASCADE
);


-- public.bans definition

-- Drop table

-- DROP TABLE public.bans;

CREATE TABLE public.bans (
	user_id serial4 NOT NULL,
	reason text NOT NULL,
	"by" serial4 NOT NULL,
	expires_at timestamp NOT NULL,
	CONSTRAINT bans_users_fk FOREIGN KEY (user_id) REFERENCES public.users(id) ON DELETE CASCADE ON UPDATE CASCADE,
	CONSTRAINT bans_users_fk_1 FOREIGN KEY ("by") REFERENCES public.users(id) ON UPDATE CASCADE
);